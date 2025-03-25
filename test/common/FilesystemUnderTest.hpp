/*
 * FilesystemUnderTest.hpp
 * Created on: 13/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */
#pragma once

#include <vfs/disk_mngr.hpp>
#include <vfs/vfs.hpp>
#include <vfs/tools/fdisk.hpp>
#include <vfs/tools/mkfs.hpp>
#include <vfs/stdstream.hpp>
#include "ram_blkdev.hpp"

namespace vfs::tests {

    class Stream : public StdStream {
    public:
        result<std::size_t> in(std::span<char> data) override;
        result<std::size_t> out(std::span<const char> data) override;
        result<std::size_t> err(std::span<const char> data) override;
    };

    class FilesystemUnderTest {
    public:
        virtual ~FilesystemUnderTest() = default;

        VirtualFS& get() const;
        VirtualFS& operator->();
        Disk&      get_disk() const;

        virtual void reload() = 0;

    protected:
        std::unique_ptr<RAMBlockDevice> block_device;
        std::unique_ptr<DiskManager>    disk_mngr;
        Disk*                           disk {};
        std::unique_ptr<VirtualFS>      vfs;

        template <typename Derived> class builder_base {
        public:
            virtual ~builder_base() = default;

            virtual std::unique_ptr<FilesystemUnderTest> create() = 0;

            Derived& set_automount();
            Derived& set_blockdev_size(std::size_t size);

        protected:
            bool        automount {};
            std::size_t blockdev_size {128 * 1024 * 1024};
        };
    };
    template <typename Derived> Derived& FilesystemUnderTest::builder_base<Derived>::set_automount()
    {
        automount = true;
        return static_cast<Derived&>(*this);
    }
    template <typename Derived> Derived& FilesystemUnderTest::builder_base<Derived>::set_blockdev_size(const std::size_t size)
    {
        blockdev_size = size;
        return static_cast<Derived&>(*this);
    }

    class ext4UnderTest : public FilesystemUnderTest {
    public:
        class Builder : public builder_base<Builder> {
        public:
            struct part_conf {
                std::size_t                  size;
                tools::fdisk::partition_conf pconf;
                tools::mkfs::ext_params      ext_params;
            };

            Builder& with_multipartition();

            std::unique_ptr<FilesystemUnderTest> create() override;

        private:
            bool multipartition {};
        };

        void reload() override;

    private:
        ext4UnderTest() = default;
    };
} // namespace vfs::tests
