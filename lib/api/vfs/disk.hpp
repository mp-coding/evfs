/*
 * disk.hpp
 * Created on: 13/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */

#pragma once

#include "blockdev.hpp"
#include "partition.hpp"

#include <vector>
#include <memory>

namespace vfs {

    class Disk final : public BlockDevice {
    public:
        Disk() = delete;

        static result<std::unique_ptr<Disk>> create(BlockDevice& device);

        Partition* borrow_partition(std::uint8_t nr);

        [[nodiscard]] auto begin() { return partitions.begin(); }
        [[nodiscard]] auto end() { return partitions.end(); }
        [[nodiscard]] auto size() const { return partitions.size(); }

        [[nodiscard]] std::error_code     probe() override;
        [[nodiscard]] std::error_code     flush() override;
        [[nodiscard]] std::error_code     write(const std::byte& buf, sector_t lba, std::size_t count) override;
        [[nodiscard]] std::error_code     read(std::byte& buf, sector_t lba, std::size_t count) override;
        [[nodiscard]] result<std::size_t> get_sector_size() const override;
        [[nodiscard]] result<sector_t>    get_sector_count() const override;
        [[nodiscard]] std::string         get_name() const override;

    private:
        explicit Disk(BlockDevice& device);

        BlockDevice&           device;
        std::vector<Partition> partitions;
        mutable std::mutex     mutex;
    };
} // namespace vfs