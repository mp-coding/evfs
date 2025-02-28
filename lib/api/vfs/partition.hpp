/*
 * partition.hpp
 * Created on: 13/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */

#pragma once

#include "blockdev.hpp"
#include "tools/mbr_partition.hpp"

namespace vfs {
    class Disk;

    class Partition final : public BlockDevice {
    public:
        Partition(Disk& disk, const tools::MBRPartition& info);

        [[nodiscard]] tools::MBRPartition get_info() const { return info; }

        [[nodiscard]] std::error_code     probe() override;
        [[nodiscard]] std::error_code     flush() override;
        [[nodiscard]] std::error_code     write(const std::byte& buf, sector_t lba, std::size_t count) override;
        [[nodiscard]] std::error_code     read(std::byte& buf, sector_t lba, std::size_t count) override;
        [[nodiscard]] result<std::size_t> get_sector_size() const override;
        [[nodiscard]] result<sector_t>    get_sector_count() const override;
        [[nodiscard]] std::string         get_name() const override;

    private:
        Disk&               disk;
        tools::MBRPartition info;

        [[nodiscard]] sector_t translate_sector(sector_t sector) const;
    };
} // namespace vfs