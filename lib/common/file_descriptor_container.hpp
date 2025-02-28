#pragma once

#include "api/vfs/filesystem.hpp"
#include "file_descriptor.hpp"

#include <unordered_map>
#include <filesystem>
#include <optional>

namespace vfs {

    /// File descriptor container that enables refcounting and delayed unlinking of file descriptors
    class file_descriptor_container {
    public:
        FileHandle&      operator[](const int index) { return *by_id[index]; }
        file_descriptor& operator[](const std::filesystem::path& path) { return by_path[path]; }

        [[nodiscard]] bool exist(const int index) const { return by_id.contains(index); }
        [[nodiscard]] bool exist(const std::filesystem::path& path) const { return by_path.contains(path); }

        [[nodiscard]] auto begin() const { return by_path.begin(); }
        [[nodiscard]] auto end() const { return by_path.end(); }
        [[nodiscard]] auto size() const { return by_path.size(); }

        /**
         * Insert a file_handle into the collection
         * @param path file path
         * @param handle fs-specific file_handle
         * @return unique file descriptor number
         */
        int insert(const std::filesystem::path& path, std::unique_ptr<FileHandle> handle)
        {
            auto [value, _] = by_path.insert({path.lexically_normal(), file_descriptor {}});
            value->second.add(idx);

            by_id.insert({idx, std::move(handle)});
            return idx++;
        }

        /**
         * Remove file descriptor from the collection
         * @param index unique file descriptor number
         * @return an optional path if just removed file descriptor needs to be unlinked
         */
        std::optional<std::filesystem::path> remove(const int index)
        {
            if (const auto it = by_id.find(index); it != by_id.end()) {
                by_id.erase(it);
                for (auto& [path, value] : by_path) {
                    if (value.remove(index)) {
                        auto       p         = std::filesystem::path {path};
                        const auto is_marked = value.is_marked_for_unlink();
                        by_path.erase(path);
                        if (is_marked) { return p; }
                        break;
                    }
                }
            }
            return {};
        }

    private:
        using by_path_map = std::unordered_map<std::string, file_descriptor>;
        using by_id_map   = std::unordered_map<int, std::unique_ptr<FileHandle>>;

        /// Special descriptors(stdin,stdout,stderr) should be also handled by vfs. For now, they are not.
        static inline int idx {3};
        by_path_map       by_path;
        by_id_map         by_id;
    };
} // namespace vfs
