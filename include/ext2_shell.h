#ifndef _FAT_SHELL_H_
#define _FAT_SHELL_H_
#define FSOPRS_TO_EXT2FS( a )      ( EXT2_FILESYSTEM* )a->pdata
#include "ext2.h"
#include "shell.h"

void shell_register_filesystem( SHELL_FILESYSTEM* );
int print_entry(SHELL_ENTRY_LIST* list);
int print_entry_long(SHELL_ENTRY_LIST* list);
int ext2_add_shell_list(EXT2_FILESYSTEM* fs, void* list, EXT2_NODE* entry);
int alpha_sort(SHELL_ENTRY_LIST* list);
int r_alpha_sort(SHELL_ENTRY_LIST* list);
int size_sort(SHELL_ENTRY_LIST* list);
int fs_write( DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, unsigned long offset, unsigned long length, const char* buffer );
int	fs_create(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name, SHELL_ENTRY* retEntry);
int fs_lookup(DISK_OPERATIONS* disk, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, const char* name);
int fs_read_dir(DISK_OPERATIONS* disk, const SHELL_ENTRY* parent, SHELL_ENTRY_LIST* list);
int fs_read(DISK_OPERATIONS* disk, EXT2_FILESYSTEM* fs, SHELL_ENTRY* entry, unsigned long offset, unsigned long length, char* buffer);
int fs_mkdir(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name, SHELL_ENTRY* retEntry);
int fs_rmdir( DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, SHELL_ENTRY *dir, char *name);
int fs_remove( DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, SHELL_ENTRY* dir, char* name );
int fs_format(DISK_OPERATIONS* disk, void* param);
int fs_chmod(DISK_OPERATIONS* disk,  SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent,  const char* name, int mode);
int fs_ls( DISK_OPERATIONS* disk, const SHELL_ENTRY* dir, SHELL_ENTRY_LIST* list, int opt);
int	fs_df( DISK_OPERATIONS* disk, SHELL_FILESYSTEM* fs);
int fs_mv( DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, SHELL_ENTRY* parent, char* name, char* dir );
int fs_cat(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, SHELL_ENTRY* dir, const char* name);
int shell_entry_to_ext2_entry(const SHELL_ENTRY* shell_entry, EXT2_NODE* ext2_entry);
int ext2_entry_to_shell_entry(EXT2_FILESYSTEM* fs, const EXT2_NODE* ext2_entry, SHELL_ENTRY* shell_entry);
#endif
