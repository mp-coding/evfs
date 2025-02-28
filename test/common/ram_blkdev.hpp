#pragma once

#include <vfs/blockdev.hpp>

namespace vfs::tests {

    class RAMBlockDevice : public BlockDevice {
    public:
        explicit RAMBlockDevice(std::size_t total_size);

        [[nodiscard]] std::error_code     probe() override;
        [[nodiscard]] std::error_code     flush() override;
        [[nodiscard]] std::error_code     write(const std::byte& buf, sector_t lba, std::size_t count) override;
        [[nodiscard]] std::error_code     read(std::byte& buf, sector_t lba, std::size_t count) override;
        [[nodiscard]] result<std::size_t> get_sector_size() const override;
        [[nodiscard]] result<sector_t>    get_sector_count() const override;
        [[nodiscard]] std::string         get_name() const override;

    private:
        static constexpr std::size_t sector_size = 512;
        const std::size_t            total_size {};

        bool                         initialized {false};
        std::unique_ptr<std::byte[]> memory;
    };
} // namespace vfs::tests
