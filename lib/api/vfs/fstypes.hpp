/*
 * fstypes.hpp
 * Created on: 13/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */

#pragma once

#include "tools/mbr_partition.hpp"

#include <string>
#include <utility>

namespace vfs::fstype {
    struct Type {
        template <typename... Args>
        constexpr explicit Type(std::string name, Args&&... args)
            : name {std::move(name)}
            , codes {args...}
        {
        }

        bool operator==(const Type& oth) const { return this->name == oth.name; }

        std::string               name;
        std::vector<std::uint8_t> codes;
    };

    const inline auto linux = Type {"linux", tools::partition_code::linux};
    const inline auto fat   = Type {"fat", tools::partition_code::vfat12, tools::partition_code::vfat16, tools::partition_code::vfat32};

} // namespace vfs::fstype

template <> struct std::hash<vfs::fstype::Type> {
    size_t operator()(const vfs::fstype::Type& k) const noexcept { return hash<std::string>()(k.name); }
}; // namespace std
