#include "fatfs_handle.hpp"
#include "logger/log.hpp"

#include <diskio.h>
#include <ctime>
#include <cassert>
#include <cinttypes>

namespace vfs {

    fatfs_handle::raw_handle& get_raw(const BYTE pdrive)
    {
        /// It is not possible to have more than one disk instance. That means, there is no support for having multiple FAT partitions(even when using different
        /// blockdevices)
        assert(pdrive == 0);
        return fatfs_handle::raw;
    }

    extern "C" {
    DSTATUS disk_initialize(const BYTE pdrv)
    {
        get_raw(pdrv).is_initialized = true;
        return RES_OK;
    }

    DSTATUS disk_status(const BYTE pdrv) { return not get_raw(pdrv).is_initialized ? RES_NOTRDY : RES_OK; }

    DRESULT disk_read(const BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
    {
        if (not get_raw(pdrv).is_initialized) { return RES_NOTRDY; }

        const auto err = get_raw(pdrv).handle->read(reinterpret_cast<std::byte&>(*buff), sector, count);
        if (err) { log_error("Sector read error errno: %i on block: %" PRIu64 "cnt: %" PRIu32, err, sector, count); }
        return err ? RES_ERROR : RES_OK;
    }

    DRESULT disk_write(const BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
    {
        if (not get_raw(pdrv).is_initialized) { return RES_NOTRDY; }

        const auto err = get_raw(pdrv).handle->write(reinterpret_cast<const std::byte&>(*buff), sector, count);
        if (err) { log_error("Sector write errno: %i on block: %" PRIu64 "cnt: %" PRIu32, err, sector, count); }
        return err ? RES_ERROR : RES_OK;
    }

    DRESULT disk_ioctl(const BYTE pdrv, const BYTE cmd, void* buff)
    {
        if (not get_raw(pdrv).is_initialized) { return RES_NOTRDY; }

        if (cmd == CTRL_SYNC) { return get_raw(pdrv).handle->flush() ? RES_ERROR : RES_OK; }
        if (cmd == CTRL_TRIM) { return RES_ERROR; }

        if (cmd == GET_SECTOR_COUNT) {
            const auto result = get_raw(pdrv).handle->get_sector_count();
            if (not result) { return RES_ERROR; }
            *static_cast<DWORD*>(buff) = *result;
            return RES_OK;
        }

        if (cmd == GET_BLOCK_SIZE) {
            const auto result = get_raw(pdrv).handle->get_sector_size();
            if (not result) { return RES_ERROR; }
            *static_cast<DWORD*>(buff) = *result;
            return RES_OK;
        }

        return RES_ERROR;
    }

    DWORD get_fattime(void)
    {
        auto tstamp = std::time(nullptr);
        tstamp      = tstamp != std::time_t {-1} ? tstamp : 0;

        tm local_time {};
        localtime_r(&tstamp, &local_time);

        /// FAT32 starts counting time from 1980/01/01 hence it can't represent valid 1970/01/01 posix time
        if (local_time.tm_year < 80) { local_time.tm_year = 80; }

        // clang-format off
        return  static_cast<DWORD>(local_time.tm_year - 80) << 25 |
                static_cast<DWORD>(local_time.tm_mon + 1) << 21 |
                static_cast<DWORD>(local_time.tm_mday) << 16 |
                static_cast<DWORD>(local_time.tm_hour) << 11 |
                static_cast<DWORD>(local_time.tm_min) << 5 |
                static_cast<DWORD>(local_time.tm_sec) >> 1;

        // clang-format on
    }
    }

    fatfs_handle::fatfs_handle(BlockDevice& handle_)
    {
        raw.handle         = &handle_;
        raw.is_initialized = false;
    }
} // namespace vfs