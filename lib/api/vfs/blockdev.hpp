/*
 * blockdev.hpp
 * Created on: 02/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */

#pragma once

#include "defs.hpp"
#include <string>

namespace vfs {
    class BlockDevice {
    public:
        using sector_t = std::uint64_t;

        virtual ~BlockDevice() = default;

        /**
         * Initialize the device
         * @return 0 in case of success, otherwise an error
         */
        [[nodiscard]] virtual std::error_code probe() = 0;

        /**
         * Sync block device's internal data
         * @return 0 in case of success, otherwise an error
         */
        [[nodiscard]] virtual std::error_code flush() = 0;

        /**
         * Write blocks
         * @param buf data to be written
         * @param lba starting block address
         * @param count how many blocks to write
         * @return 0 in case of success, otherwise an error
         */
        [[nodiscard]] virtual std::error_code write(const std::byte& buf, sector_t lba, std::size_t count) = 0;
        /**
         * Read blocks
         * @param buf where to store read data
         * @param lba starting block address
         * @param count how many blocks to read
         * @return 0 in case of success, otherwise an error
         */
        [[nodiscard]] virtual std::error_code read(std::byte& buf, sector_t lba, std::size_t count) = 0;

        [[nodiscard]] virtual result<std::size_t> get_sector_size() const  = 0;
        [[nodiscard]] virtual result<sector_t>    get_sector_count() const = 0;
        [[nodiscard]] virtual std::string         get_name() const         = 0;
    };
} // namespace vfs
