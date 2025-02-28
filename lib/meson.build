subdir('config')
subdir('third-party/lwext4')
subdir('third-party/fatfs')

_inc = include_directories(['.','api'])

src = [
    'common/disk.cpp',
    'common/partition.cpp',
    'common/disk_mngr.cpp',
    'common/vfs.cpp',
    'logger/logger.cpp',
    'tools/fdisk.cpp',
    'tools/mkfs.cpp',
    'fstypes/filesystem.cpp',
    'fstypes/handle/lwext4_handle.cpp',
    'fstypes/handle/fatfs_handle.cpp',
    'fstypes/filesystem_lwext4.cpp',
]

deps_public = []
deps_private = [lwext4_dep, libfatfs_dep]

c_opt_args = ['-Wno-psabi', '-Wno-c99-extensions']

libvfs = library('vfs',
                 sources : src,
                 include_directories : _inc,
                 dependencies : [deps_public, deps_private],
                 c_args : c_opt_args,
                 cpp_args : c_opt_args
)

libvfs_dep = declare_dependency(
    link_with : libvfs,
    include_directories : ['api'],
    dependencies : deps_public
)



