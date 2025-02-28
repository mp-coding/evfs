/**
 * initializers
 * Created on: 28/02/2025
 * Author mateuszpiesta
 * Company: mprogramming
 */

#pragma once

#include "FilesystemUnderTest.hpp"

#include <memory>

namespace vfs::tests {
    struct ext4_initializer;
    struct fat_initializer;

    template <typename T> struct initializer {
        std::unique_ptr<FilesystemUnderTest> operator()() const
        {
            if constexpr (std::is_same_v<T, ext4_initializer>) { return ext4UnderTest::Builder {}.set_automount().create(); }

            return nullptr;
        }
    };
} // namespace vfs::tests
