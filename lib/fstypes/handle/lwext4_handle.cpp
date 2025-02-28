#include "lwext4_handle.hpp"

#include "logger/log.hpp"
#include <cstring>
#include <cinttypes>

namespace vfs {

    int lwext4_handle::write(ext4_blockdev* bdev, const void* buf, const std::uint64_t blk_id, const std::uint32_t blk_cnt)
    {
        const auto ctx = static_cast<lwext4_handle*>(bdev->bdif->p_user);
        if (!ctx) { return -EIO; }
        const auto err = ctx->device.write(*static_cast<const std::byte*>(buf), blk_id, blk_cnt);
        if (err) { log_error("Sector write error errno: %i on block: %" PRIu64 "cnt: %" PRIu32, err, blk_id, blk_cnt); }
        return err.value();
    }

    int lwext4_handle::read(ext4_blockdev* bdev, void* buf, const std::uint64_t blk_id, const std::uint32_t blk_cnt)
    {
        const auto ctx = static_cast<lwext4_handle*>(bdev->bdif->p_user);
        if (!ctx) { return -EIO; }
        const auto err = ctx->device.read(*static_cast<std::byte*>(buf), blk_id, blk_cnt);
        if (err) { log_error("Sector read error errno: %i on block: %" PRIu64 "cnt: %" PRIu32, err, blk_id, blk_cnt); }
        return err.value();
    }

    int lwext4_handle::open(ext4_blockdev*) { return 0; }

    int lwext4_handle::close(ext4_blockdev*) { return 0; }

    lwext4_handle::lwext4_handle(BlockDevice& handle)
        : device {handle}
    {
        const auto sect_size = device.get_sector_size();
        if (not sect_size) { log_error("Unable to get sector size: %s", sect_size.error().message().c_str()); }
        const auto sect_count = device.get_sector_count();
        if (not sect_count) { log_error("Unable to get sector count: %s", sect_count.error().message().c_str()); }

        std::memset(&ifc, 0, sizeof(ifc));
        std::memset(&bdev, 0, sizeof(bdev));
        ifc.open         = open;
        ifc.bread        = read;
        ifc.bwrite       = write;
        ifc.close        = close;
        buf              = std::make_unique<std::uint8_t[]>(*sect_size);
        ifc.ph_bbuf      = buf.get();
        ifc.ph_bcnt      = *sect_count;
        ifc.ph_bsize     = *sect_size;
        ifc.p_user       = this;
        bdev.bdif        = &ifc;
        bdev.part_offset = 0;
        bdev.part_size   = sect_size.value() * sect_count.value();
    }

    ext4_blockdev& lwext4_handle::get_blockdev() { return bdev; }
    std::string    lwext4_handle::get_name() const { return device.get_name(); }
} // namespace vfs
