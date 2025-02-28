#include "api/vfs/disk_mngr.hpp"
#include "api/vfs/disk.hpp"
#include <expected>

namespace vfs {

    DiskManager::DiskManager()  = default;
    DiskManager::~DiskManager() = default;
    result<Disk*> DiskManager::register_device(BlockDevice& blkdev)
    {
        auto_lock lock {m_lock};

        const auto name = blkdev.get_name();

        if (const auto ret = m_disk_map.find(name); ret != std::end(m_disk_map)) { return error(EINVAL); }
        if (const auto ret = blkdev.probe()) { return error(ret); }

        if (auto disk = Disk::create(blkdev); not disk) {
            return error(disk.error());
        } else {
            auto [fst, snd] = m_disk_map.emplace(name, std::move(disk.value()));
            return &*fst->second;
        }
    }
    std::error_code DiskManager::unregister_device(std::string_view device_name)
    {
        auto_lock lock {m_lock};

        const auto it = m_disk_map.find(std::string(device_name));
        if (it == std::end(m_disk_map)) { return from_errno(ENOENT); }
        m_disk_map.erase(it);
        return {};
    }
    Disk* DiskManager::get(const std::string& disk)
    {
        if (m_disk_map.contains(disk)) { return m_disk_map[disk].get(); }
        return nullptr;
    }

} // namespace vfs