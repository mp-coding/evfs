/**
 * stdstream
 * Created on: 11/03/2025
 * Author mateuszpiesta
 * Company: mprogramming
 */

#pragma once

#include "defs.hpp"

#include <span>

namespace vfs {
    class StdStream {
    public:
        virtual ~StdStream()                                        = default;
        virtual result<std::size_t> in(std::span<char> data)        = 0;
        virtual result<std::size_t> out(std::span<const char> data) = 0;
        virtual result<std::size_t> err(std::span<const char> data) = 0;
    };
} // namespace vfs