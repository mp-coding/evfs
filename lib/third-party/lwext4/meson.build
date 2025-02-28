lwext4_inc = include_directories('include')

lwext4_src = files (
    'src/ext4_balloc.c',
    'src/ext4_bcache.c',
    'src/ext4_bitmap.c',
    'src/ext4_block_group.c',
    'src/ext4_blockdev.c',
    'src/ext4_crc32.c',
    'src/ext4_debug.c',
    'src/ext4_dir_idx.c',
    'src/ext4_dir.c',
    'src/ext4_fs.c',
    'src/ext4_hash.c',
    'src/ext4_ialloc.c',
    'src/ext4_inode.c',
    'src/ext4_journal.c',
    'src/ext4_mbr.c',
    'src/ext4_mkfs.c',
    'src/ext4_super.c',
    'src/ext4_trans.c',
    'src/ext4.c'
)

lwext4_args = [
    '-fdata-sections',
    '-ffunction-sections',
    '-Wno-format',
    '-Wno-unused-variable',
    '-Wno-pedantic',
    '-Wno-unused-function',
    '-Wno-unused-but-set-variable',
    '-Wno-c99-extensions',
]

lwext4 = library('lwext4', sources : lwext4_src, include_directories : lwext4_inc, c_args : lwext4_args, dependencies : lwext4_config_dep)
lwext4_dep = declare_dependency(
    link_with : lwext4,
    include_directories : lwext4_inc,
    dependencies : lwext4_config_dep,
)
