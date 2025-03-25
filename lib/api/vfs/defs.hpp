/*
 * defs.hpp
 * Created on: 02/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */

#pragma once

#include <bitset>
#include <mutex>
#include <system_error>
#include <expected>

namespace vfs {
    template <typename T> using result = std::expected<T, std::error_code>;

    using auto_lock = std::lock_guard<std::mutex>;

    struct MountFlags {
        enum {
            read_only = 0,
            remount   = 5,
        };
    };
    using Flags = std::bitset<32>;

    inline std::error_code from_errno(const int err) { return {err, std::generic_category()}; }
    inline auto            error(const int err) { return std::unexpected(from_errno(err)); }
    inline auto            error(const std::error_code err) { return std::unexpected(err); }

    /// Root base name used by auto-mount when underlying filesystem does not provide info about partition's name/label. In that case, root_base will be used to
    /// create a unique partition's mount point name.
    constexpr auto root_base = "/volume";
} // namespace vfs
