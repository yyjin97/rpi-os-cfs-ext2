#ifndef _SHELL_H_
#define _SHELL_H_

#include "disk.h"

typedef struct
{
	unsigned short	year;
	unsigned char	month;
	unsigned char	day;

	unsigned char	hour;
	unsigned char	minute;
	unsigned char	second;
} SHELL_FILETIME;

typedef struct SHELL_ENTRY
{
	struct SHELL_ENTRY*	parent;

	unsigned char		name[256];
	unsigned char		isDirectory;
	unsigned int		size;

	unsigned short		permition;
	SHELL_FILETIME		createTime;
	SHELL_FILETIME		modifyTime;

	UINT16 magic_signature;
	char				pdata[1024];
} SHELL_ENTRY;

typedef struct SHELL_ENTRY_LIST_ITEM
{
	struct SHELL_ENTRY				entry;
	struct SHELL_ENTRY_LIST_ITEM*	next;
} SHELL_ENTRY_LIST_ITEM;

typedef struct
{
	unsigned int					count;
	SHELL_ENTRY_LIST_ITEM*			first;
	SHELL_ENTRY_LIST_ITEM*			last;
} SHELL_ENTRY_LIST;

struct SHELL_FILE_OPERATIONS;

typedef struct SHELL_FS_OPERATIONS
{
	int ( *ls)( DISK_OPERATIONS*, const SHELL_ENTRY*, SHELL_ENTRY_LIST* , int );
	int	( *read_dir )( DISK_OPERATIONS*, const SHELL_ENTRY*, SHELL_ENTRY_LIST* );
	int	( *df )( DISK_OPERATIONS*, struct SHELL_FILESYSEM* );
	int	( *mv )( DISK_OPERATIONS*, struct SHELL_FS_OPERATIONS*, SHELL_ENTRY*, const char*, const char* );
	int ( *mkdir )( DISK_OPERATIONS*, struct SHELL_FS_OPERATIONS*, const SHELL_ENTRY*, const char*, SHELL_ENTRY* );
	int ( *rmdir )( DISK_OPERATIONS*, struct SHELL_FS_OPERATIONS*, const SHELL_ENTRY*, const char* );
	int ( *lookup )( DISK_OPERATIONS*, const SHELL_ENTRY*, SHELL_ENTRY*, const char* );
	int ( *cat )( DISK_OPERATIONS*, struct SHELL_FS_OPERATIONS*, SHELL_ENTRY*, const char* );
	int ( *chmod )(DISK_OPERATIONS*, struct SHELL_FS_OPERATIONS*, const SHELL_ENTRY*,  const char*, int );

	struct SHELL_FILE_OPERATIONS*	fileOprs;
	void*	pdata;
} SHELL_FS_OPERATIONS;

typedef struct SHELL_FILE_OPERATIONS
{
	int	( *create )( DISK_OPERATIONS*, SHELL_FS_OPERATIONS*, const SHELL_ENTRY*, const char*, SHELL_ENTRY* );
	int ( *remove )( DISK_OPERATIONS*, SHELL_FS_OPERATIONS*, const SHELL_ENTRY*, const char* );
	int	( *read )( DISK_OPERATIONS*, SHELL_FS_OPERATIONS*, const SHELL_ENTRY*, unsigned long, unsigned long, char* );
	int	( *write )( DISK_OPERATIONS*, SHELL_FS_OPERATIONS*, const SHELL_ENTRY*, SHELL_ENTRY*, unsigned long, unsigned long, const char* );
} SHELL_FILE_OPERATIONS;

typedef struct SHELL_FILESYSTEM
{
	char*	name;
	int		( *mount )( DISK_OPERATIONS*, SHELL_FS_OPERATIONS*, SHELL_ENTRY* );
	void	( *umount )( SHELL_FS_OPERATIONS* );
	int		( *format )( DISK_OPERATIONS*, void* );
} SHELL_FILESYSTEM;

int		init_entry_list( SHELL_ENTRY_LIST* list );
int		add_entry_list(SHELL_ENTRY_LIST* list, SHELL_ENTRY_LIST_ITEM* new);
int	release_entry_list( SHELL_ENTRY_LIST* );
int shell_process(void);

#endif
