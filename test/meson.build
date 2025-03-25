catch2 = subproject('catch2', default_options : ['tests=false'])
catch2_with_main_dep = catch2.get_variable('catch2_with_main_dep')
catch2_dep = catch2.get_variable('catch2_dep')

test_common = library('test-common',
                      sources : ['common/ram_blkdev.cpp', 'common/FilesystemUndertest.cpp'],
                      include_directories : '',
                      dependencies : [vfs_dep],
)

test_common_dep = declare_dependency(
    link_with : test_common,
    include_directories : 'common',
    dependencies : vfs_dep
)

fd_test = executable('FileDescriptors', 'fd_test.cpp', dependencies : [test_common_dep, catch2_with_main_dep])
test('FileDescriptors', fd_test)
#
dir_test = executable('Directories', 'dir_test.cpp', dependencies : [test_common_dep, catch2_with_main_dep])
test('Directories', dir_test)
#
#tools_test = executable('Tools', 'tools_test.cpp', dependencies : [test_common_dep, catch2_with_main_dep])
#test('Tools', tools_test)
#
vfs_test = executable('VFS', 'vfs_test.cpp', dependencies : [test_common_dep, catch2_with_main_dep])
test('VFS', vfs_test)
#
#mt_test = executable('Multithreading', 'multithreading_test.cpp', dependencies : [test_common_dep, catch2_with_main_dep])
#test('Multithreading', mt_test)
#
syscalls_test = executable('Syscalls', 'syscalls_test.cpp',
                           dependencies : [test_common_dep, catch2_with_main_dep, vfs_syscalls_dep]

)
test('Syscalls', syscalls_test)