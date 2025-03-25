subdir('config')
subdir('third-party/lwext4')

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
    'fstypes/filesystem_lwext4.cpp',
]

deps_public = []
deps_private = [lwext4_dep]

c_opt_args = ['-Wno-psabi']

libevfs = library('evfs',
                 sources : src,
                 include_directories : _inc,
                 dependencies : [deps_public, deps_private],
                 c_args : c_opt_args,
                 cpp_args : c_opt_args
)

evfs_dep = declare_dependency(
    link_with : libevfs,
    include_directories : ['api'],
    dependencies : deps_public
)



