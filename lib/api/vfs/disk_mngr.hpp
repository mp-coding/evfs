/*
 * disk_mngr.hpp
 * Created on: 13/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */

#pragma once

#include "defs.hpp"
#include "blockdev.hpp"

#include <unordered_map>
#include <memory>

namespace vfs {

    class Disk;

    class DiskManager {
    public:
        DiskManager();
        ~DiskManager();
        DiskManager(const DiskManager&)                    = delete;
        auto operator=(const DiskManager&) -> DiskManager& = delete;

        result<Disk*>   register_device(BlockDevice& blkdev);
        std::error_code unregister_device(std::string_view device_name);

        [[nodiscard]] auto        begin() { return m_disk_map.begin(); }
        [[nodiscard]] auto        end() { return m_disk_map.end(); }
        [[nodiscard]] std::size_t size() const { return m_disk_map.size(); }

        Disk* get(const std::string& disk);

    private:
        std::unordered_map<std::string, std::unique_ptr<Disk>> m_disk_map;
        Mutex                                                  m_lock;
    };

} // namespace vfs
