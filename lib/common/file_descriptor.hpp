#pragma once

#include <list>
#include <algorithm>

namespace vfs {
    class file_descriptor {
    public:
        void               add(const int id) { indexes.push_back(id); }
        [[nodiscard]] bool remove(const int id)
        {
            if (const auto it = std::find(indexes.begin(), indexes.end(), id); it != indexes.end()) { indexes.erase(it); }
            return is_empty();
        }

        [[nodiscard]] auto begin() const { return indexes.begin(); }
        [[nodiscard]] auto end() const { return indexes.end(); }
        [[nodiscard]] auto size() const { return indexes.size(); }

        void mark_for_unlink() { marked_for_unlink = true; }

        [[nodiscard]] bool is_empty() const noexcept { return indexes.empty(); }
        [[nodiscard]] bool is_marked_for_unlink() const noexcept { return marked_for_unlink; }

    private:
        std::list<int> indexes;
        bool           marked_for_unlink {false};
    };
} // namespace vfs
