//#include <stdio.h>
//#include <stdlib.h>
//#include <memory.h>

#include "ext2_shell.h"
#include "printf.h"
#include "mm.h"
typedef struct {
	char * address;
}DISK_MEMORY;
void printFromP2P(char * start, char * end);

int fs_dumpDataSector(DISK_OPERATIONS* disk, int usedSector)
{
	//for(int j = 0; j < NUMBER_OF_GROUPS; j++) {
	//printf("group %u : \n", j);
	char *start = ((DISK_MEMORY *)disk->pdata)->address + 2 * disk->bytesPerSector + 0 * 15000 * disk->bytesPerSector + 2 * 4 * disk->bytesPerSector;
	for(int i = 0; i < MAX_BLOCK_SIZE;i++) {
		//fprintf(stdout, "%02X  ", *(unsigned char *)start);
		printf("%02X  ", *(unsigned char *)start);
		start++;
	}
	printf("\n");
	//}
	return EXT2_SUCCESS;

}

void printf_by_sel(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, const char* name, int sel, int num)
{
	
	switch (sel) {
	case 1:
		fs_dumpDataSector(disk, 2);
		break;
		}
		/*
	case 2:
		fs_dumpDataSector(disk, 2);
		break;
	case 3:
		fs_dumpDataSector(disk, 3);
		break;
	case 4:
		fs_dumpDataSector(disk, 4);
		break;
	case 5:
		fs_dumpDataSector(disk, 5);
		fs_dumpDataSector(disk, 6);
		break;
	case 6:
		fs_dumpDataPart(disk, fsOprs, parent, entry, name);
		break;
	case 7:
		fs_dumpfileinode(disk, fsOprs, parent, entry, name);
		break;
	case 8:
		fs_dumpDataBlockByNum(disk, fsOprs, parent, entry, num);
		break;
	}
	*/
}
int fs_format(DISK_OPERATIONS* disk, void* param)
{
	if((char *)param)
		printf("formatting as a %s\n\r", (char *)param);
	ext2_format(disk);

	return  1;
}

//static SHELL_FILE_OPERATIONS g_file =
SHELL_FILE_OPERATIONS g_file =
{
	fs_create,
	fs_remove,
	NULL,
	fs_write
};

//static SHELL_FS_OPERATIONS   g_fsOprs =
SHELL_FS_OPERATIONS   g_fsOprs =
{
	fs_ls,
	fs_read_dir,
	fs_df,
	fs_mv,
	fs_mkdir,
	fs_rmdir,
	fs_lookup,
	fs_cat,
	fs_chmod,
	&g_file,
	NULL
};

int fs_mount(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, SHELL_ENTRY* root)
{
	EXT2_FILESYSTEM* fs;
	EXT2_NODE root_node;

	*fsOprs = g_fsOprs;
	//fsOprs->pdata = (EXT2_FILESYSTEM* )malloc(sizeof(EXT2_FILESYSTEM));
	fsOprs->pdata = (EXT2_FILESYSTEM*)allocate_kernel_page();
	fs = FSOPRS_TO_EXT2FS(fsOprs);
	ZeroMemory(fs,sizeof(EXT2_FILESYSTEM));  
	fs->disk = disk;

	if(read_superblock(fs) < 0) {
		printf("read superblock error\n\r");
		return EXT2_ERROR;
	}

	if(fill_node_struct(fs, &root_node) < 0) {
		printf("fill shell entry error\n\r");
		return EXT2_ERROR;
	}

	ext2_entry_to_shell_entry(fs, &root_node, root);

	/* mount 정보 출력 */
	printf("volume name        : %s\n\r", root->name);
	printf("number of sector   : %u\n\r", fs->disk->numberOfSectors);
	printf("bytes per sector   : %u\n\r", fs->disk->bytesPerSector);
	printf("inode count        : %u\n\r", fs->sb.inode_count);
	printf("block count        : %u\n\r", fs->sb.block_count);
	printf("directory count    : %u\n\n\r", fs->gd.directories_count);

	return EXT2_SUCCESS;

}

void fs_umount(SHELL_FS_OPERATIONS* fsOprs)
{
	if(fsOprs->pdata)
	{
		//free(fsOprs->pdata);
		fsOprs->pdata =0;
	}
	printf("umount completed\n\r");
	return;
	
}

//static SHELL_FILESYSTEM g_ext2 =
SHELL_FILESYSTEM g_ext2 =
{
	"EXT2",
	fs_mount,
	fs_umount,
	fs_format
};

/* 'entry'에 해당하는 ext2 node를 shell list에 저장하는 함수 */
int ext2_add_shell_list(EXT2_FILESYSTEM* fs, void* list, EXT2_NODE* entry)
{
	SHELL_ENTRY s_entry;
	//SHELL_ENTRY_LIST_ITEM* s_item = (SHELL_ENTRY_LIST_ITEM *) malloc(sizeof(SHELL_ENTRY_LIST_ITEM));
	SHELL_ENTRY_LIST_ITEM* s_item = (SHELL_ENTRY_LIST_ITEM *)allocate_kernel_page();

	ext2_entry_to_shell_entry(fs, entry, &s_entry);

	memcpy(&s_item->entry, &s_entry, sizeof(SHELL_ENTRY));

	if(add_entry_list(list, s_item) < 0) {
		printf("add entry list error\n\r");
		return -1;
	}

	return 0;
}

int	fs_df( DISK_OPERATIONS* disk, SHELL_FILESYSTEM* fs)
{
	EXT2_SUPER_BLOCK sb;
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 blocks;
	UINT32 used;
	UINT32 free;
	int used_per;
	int free_per;
	int used_per_point;
	int free_per_point;

	ZeroMemory(block, sizeof(block));
	block_read(disk, 0, 0, block);
	memcpy(&sb, block, sizeof(EXT2_SUPER_BLOCK));

	blocks = sb.block_count;
	used = sb.block_count - sb.free_block_count;
	free = sb.free_block_count;
	used_per = (used*100) / blocks;				//float값 사용할 수 없으므로 정수부분과 소수점 나눠서 구함 
	free_per = (free*100) / blocks;
	used_per_point = ((used*10000)/blocks) % 100;
	free_per_point = ((free*10000)/blocks) % 100;

	printf("\n\rFILE SYSTEM      BLOCKS      USED      USED%%      FREE      FREE%% \n\r");
	printf("%11s      %6u     %5u   %4d.%d%%      %4u    %4d.%d%%\n\r", fs->name, blocks, used, used_per, used_per_point, free, free_per, free_per_point);
	printf("\n\rblock size	%u (byte)\n\r", (0x01 << sb.log_block_size)*1024);

	return EXT2_SUCCESS;
}

int fs_mv(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, SHELL_ENTRY* parent, char* name, char* dir)
{
	SHELL_ENTRY shell_dir;
	EXT2_NODE entry;
	EXT2_NODE directory;
	EXT2_NODE ext2_parent;
	EXT2_DIR_ENTRY_LOCATION location;

	shell_entry_to_ext2_entry(parent, &ext2_parent);

	if(ext2_lookup(&ext2_parent, name, &entry) == 1) {				//이동하려는 파일이 존재하는지 검사 
		printf("No %s file\n\r", name);
		return -1;
	}

	if(ext2_lookup(&ext2_parent, dir, &directory) == 1) {			//이동하려는 디렉토리가 존재하는지 검사
		printf("No %s directory\n\r", dir);
		return -1;
	}

	ext2_entry_to_shell_entry(fsOprs->pdata, &directory, &shell_dir);

	if(shell_dir.isDirectory != 1) {								//찾은 디렉토리가 디렉토리 파일이 맞는지 검사 
		printf("%s file is not a directory file\n\r", dir);
		return -1;
	}

	release_dir_entry(fsOprs->pdata, &ext2_parent, &entry);				//원래 존재하던 디렉토리에서 이동하려는 파일의 directory entry 삭제 
	insert_entry(fsOprs->pdata, &directory, &entry.entry, &location);

	return EXT2_SUCCESS;

}

int fs_cat(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, SHELL_ENTRY* dir, const char* name)
{
	SHELL_ENTRY entry;
	char* buff;
	char* buf;

	if(fs_lookup(disk, dir, &entry, name) == 1) {
		printf("No %s file\n\r", name);
		return -1;
	}

	if(entry.isDirectory == 1) {
		printf("%s is a directory file\n\r", entry.name);
		return EXT2_ERROR;
	}

	//buff = malloc(entry.size);
	buff = allocate_kernel_page();
	buf = buff;
	ZeroMemory(buff, entry.size);

	printf("%s(size : %u):\n\r", name, entry.size);
	if(fs_read(disk, fsOprs->pdata, &entry, 0, entry.size, buf) < 0)
		return -1;

	for(int i = 0; i < entry.size; i++) {
		printf("%c",*(buf+i));
	} 
	printf("\n\r");

	//free(buff);
	free_page(buff);

	return EXT2_SUCCESS;
}

int fs_read(DISK_OPERATIONS* disk, EXT2_FILESYSTEM* fs, SHELL_ENTRY* entry, unsigned long offset, unsigned long length, char* buffer)
{
	EXT2_NODE ext2_entry;

	shell_entry_to_ext2_entry(entry, &ext2_entry);

	if(ext2_read(&ext2_entry, offset, length, buffer) < 0) 
		return EXT2_ERROR;

	return EXT2_SUCCESS;

}

int fs_write(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, unsigned long offset, unsigned long length, const char* buffer)
{
	EXT2_NODE   EXT2Entry;
	int			 result;

	shell_entry_to_ext2_entry(entry, &EXT2Entry);

    result = ext2_write(&EXT2Entry, offset, length, buffer);

	ext2_entry_to_shell_entry(EXT2Entry.fs, &EXT2Entry, entry);

	return result;
}

void shell_register_filesystem(SHELL_FILESYSTEM* fs)
{
	*fs = g_ext2;    //shell filesystem 초기화 (파일시스템 이름,함수포인터)
}

int	fs_create(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name, SHELL_ENTRY* retEntry)
{
	EXT2_NODE	EXT2Parent;
	EXT2_NODE	EXT2Entry;
	int				result;

	shell_entry_to_ext2_entry(parent, &EXT2Parent);

	result = ext2_create(&EXT2Parent, name, &EXT2Entry);

	ext2_entry_to_shell_entry(EXT2Parent.fs, &EXT2Entry, retEntry);

	return result;
}

int shell_entry_to_ext2_entry(const SHELL_ENTRY* shell_entry, EXT2_NODE* ext2_entry)
{
	memcpy(ext2_entry, shell_entry->pdata, sizeof(EXT2_NODE));

	return EXT2_SUCCESS;
}

int ext2_entry_to_shell_entry(EXT2_FILESYSTEM* fs, const EXT2_NODE* ext2_entry, SHELL_ENTRY* shell_entry)		//ext2 entry를 shell entry에 복사
{
	INODE inode;

	strcpy((char *)shell_entry->name, (char *)ext2_entry->entry.name);

	get_inode(fs->disk, &fs->sb, ext2_entry->entry.inode, &inode);

	if((inode.mode & 0x4000) != 0) 
		shell_entry->isDirectory = 1;
	else 
		shell_entry->isDirectory = 0;

	shell_entry->permition = inode.mode & 0x1ff;
	shell_entry->size = inode.size;

	memcpy(shell_entry->pdata, ext2_entry, sizeof(EXT2_NODE));
		
	return EXT2_SUCCESS;
}


int fs_lookup(DISK_OPERATIONS* disk, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, const char* name)
{
	EXT2_NODE	EXT2Parent;
	EXT2_NODE	EXT2Entry;
	int result;

	shell_entry_to_ext2_entry(parent, &EXT2Parent);

	if (result = ext2_lookup(&EXT2Parent, name, &EXT2Entry)) return result;

	ext2_entry_to_shell_entry(EXT2Parent.fs, &EXT2Entry, entry);

	return result;
}

int fs_read_dir(DISK_OPERATIONS* disk, const SHELL_ENTRY* parent, SHELL_ENTRY_LIST* list)
{
	EXT2_NODE parent_node;

	if(!parent->isDirectory) return EXT2_ERROR;

	shell_entry_to_ext2_entry(parent, &parent_node);

	if(ext2_read_dir(&parent_node, ext2_add_shell_list, list) != EXT2_SUCCESS) 
		return EXT2_ERROR;

	return EXT2_SUCCESS;

}

int is_exist(DISK_OPERATIONS* disk, const SHELL_ENTRY* parent, const char* name) 	//name에 해당하는 파일이름이 존재하는지 확인하고 존재하면 1 리턴
{
	SHELL_ENTRY_LIST list;
	SHELL_ENTRY_LIST_ITEM* item;
	unsigned int count = 0;

	init_entry_list(&list);

	if(fs_read_dir(disk, parent, &list) != EXT2_SUCCESS) 
		return EXT2_ERROR;

	count = list.count;
	item = list.first;

	for(int i = 0; i < count; i++) {
		if(strcmp((const char *)item->entry.name, (const char *)name) == 0) {
			release_entry_list(&list);
			return 1; 
		}
		item = item->next;
	}

	release_entry_list(&list);

	return 0;
}

int fs_mkdir(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name, SHELL_ENTRY* retEntry)
{
	EXT2_NODE ext2_parent;
	EXT2_NODE ext2_entry;

	if(is_exist(disk, parent, name)) {
		printf("%s file name already exists\n\r", name);
		return 0;
	}

	shell_entry_to_ext2_entry(parent, &ext2_parent);

	ext2_entry.fs = ext2_parent.fs;

	if(ext2_mkdir(&ext2_parent, name, &ext2_entry) < 0) {
		printf("mkdir error\n\r");
		return EXT2_ERROR;
	}

	ext2_entry_to_shell_entry(fsOprs->pdata, &ext2_entry, retEntry);

	return EXT2_SUCCESS;
}

int fs_rmdir( DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, SHELL_ENTRY *dir, char *name)
{
	SHELL_ENTRY entry;
	EXT2_NODE node;
	EXT2_NODE p_node;
	INODE inode;

	if(fs_lookup(disk, dir, &entry, name)  == 1) {
		printf("NO %s directory file\n\r", name);
		return -1;
	}

	if(entry.isDirectory == 0) {
		printf("%s is not a directory file\n\r", name);
		return -1;
	}

	shell_entry_to_ext2_entry(&entry, &node);
	shell_entry_to_ext2_entry(dir, &p_node);

	get_inode(disk, &p_node.fs->sb, node.entry.inode, &inode);
	if(inode.size > 2 * sizeof(EXT2_DIR_ENTRY)) {							//directory 파일 내에 파일이 존재하는 경우 삭제 불가 
		printf("rmdir: failed to remove '%s': Directory not empty\n\r", node.entry.name);
		return -1;
	}

	get_inode(disk, &p_node.fs->sb, p_node.entry.inode, &inode);
	inode.links_count -= 1;
	set_inode(disk, &p_node.fs->sb, p_node.entry.inode, &inode);

	if(release_dir_entry(fsOprs->pdata, &p_node, &node) < 0) {
		printf("release dir entry error\n\r");
		return -1;
	}

	if(release_inode(fsOprs->pdata, &node) < 0) {
		printf("release inode error\n\r");
		return -1;
	}

	return EXT2_SUCCESS;
	
}

int fs_remove( DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, SHELL_ENTRY* dir, char* name )
{
	SHELL_ENTRY entry;
	EXT2_NODE ext2_entry;
	EXT2_NODE ext2_dir;

	if(fs_lookup(disk, dir, &entry, name) == 1) {
		printf("No %s file\n\r", name);
		return -1;
	} 

	if(entry.isDirectory == 1) {
		printf("%s file is directory file\n\r", name);
		return -1;
	}

	shell_entry_to_ext2_entry(dir, &ext2_dir);
	shell_entry_to_ext2_entry(&entry, &ext2_entry);

	if(release_dir_entry(fsOprs->pdata, &ext2_dir, &ext2_entry) < 0) {
		printf("release directory entry error\n\r");
		return EXT2_ERROR;
	}

	if(release_inode(fsOprs->pdata, &ext2_entry) < 0) {
		printf("release file error\n\r");
		return EXT2_ERROR;
	}

	return EXT2_SUCCESS;

}

int fs_chmod(DISK_OPERATIONS* disk,  SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent,  const char* name, int mode)
{
    SHELL_ENTRY entry;
	EXT2_NODE ext2_entry;
	EXT2_NODE ext2_dir;
	int result;

	result = fs_lookup(disk,parent,&entry,name);
	
	if(result != EXT2_SUCCESS)
	{
		printf(" %s file doesn't exist\n\r", name);
		return EXT2_ERROR;
	}

	shell_entry_to_ext2_entry(parent, &ext2_dir);
	shell_entry_to_ext2_entry(&entry, &ext2_entry);

   if(ext2_chmod(disk,fsOprs->pdata,&ext2_dir,&ext2_entry,mode)==EXT2_ERROR)
	{
	   printf("wrong mode number!\n\r");
	   return EXT2_ERROR;
	}

	return EXT2_SUCCESS;

}

int fs_ls( DISK_OPERATIONS* disk, const SHELL_ENTRY* dir, SHELL_ENTRY_LIST* list, int opt)
{
	SHELL_ENTRY_LIST_ITEM* item;
	SHELL_ENTRY_LIST_ITEM* release;

	if(fs_read_dir(disk, dir, list) < 0) {
		return -1;
	};

	if(!(opt & 0x0001)) {			//-a 옵션이 없는 경우 "."과 ".." 디렉토리를 list에서 제거 
		release = list->first;
		item = list->first;
		item = item->next;
		item = item->next;
		list->first = item;
		//free(release->next);
		//free(release);
		free_page((unsigned long)release->next);
		free_page((unsigned long)release);
		list->count -= 2;
	}

	if(opt & 0x0004) size_sort(list);				//-S 옵션이 있는 경우 size 순서로 sorting
	else if(opt & 0x0008) r_alpha_sort(list);		//-r 옵션이 있는 경우 알파벳 역순으로 sorting 
	else alpha_sort(list);							//ls 명령어는 알파벳순서가 default임

	if(opt & 0x0002) print_entry_long(list);		//-l 옵션이 있는 경우 파일 정보를 자세하게 출력 
	else print_entry(list);							//-l 옵션이 없는 경우 파일 이름만 출력 

	return EXT2_SUCCESS;
	
}

int print_per(SHELL_ENTRY_LIST_ITEM* item)
{
	unsigned short permition = item->entry.permition;

	if(item->entry.isDirectory) printf("d");
	else printf("-");

	for(int i = 0; i < 3; i++) {
		if(permition & 0x0100) printf("r");
		else printf("-");

		if(permition & 0x0080) printf("w");
		else printf("-");

		if(permition & 0x0040) printf("x");
		else printf("-");

		permition = permition << 3;
	}

	return EXT2_SUCCESS;
}

int print_entry_long(SHELL_ENTRY_LIST* list)
{
	SHELL_ENTRY_LIST_ITEM* item;
	EXT2_NODE entry;
	INODE inode;
	char time[25];

	item = list->first;

	for(int i = 1; i <= list->count; i++) {
		shell_entry_to_ext2_entry(&item->entry, &entry);
		get_inode(entry.fs->disk, &entry.fs->sb, entry.entry.inode, &inode);
		print_per(item);
		printf(" %3u %4x %4x %5u ", inode.links_count, inode.uid, inode.gid, inode.size);
		//memcpy(time, ctime(&inode.mtime), 25);
		time[0] = NULL;
		time[24] = NULL;
		printf("%s ", time);
		printf("%s \n\r", entry.entry.name);
		item = item->next;
	}

	return EXT2_SUCCESS;
}

int print_entry(SHELL_ENTRY_LIST* list)
{
	SHELL_ENTRY_LIST_ITEM* item;

	item = list->first;
	for(int i = 1; i <= list->count; i++) {
		printf(" %s	", item->entry.name);
		if(i%6 == 0) printf("\n\r");
		item = item->next;
	}
	printf("\n\r");

	return EXT2_SUCCESS;
}

int swap(SHELL_ENTRY_LIST* list, int a, int b) 	//list에서 a번째 entry와 b번째 entry를 swap
{
	SHELL_ENTRY_LIST_ITEM* item_a;
	SHELL_ENTRY_LIST_ITEM* item_b;
	SHELL_ENTRY_LIST_ITEM* item;
	int i = 0;

	item_a = list->first;

	while( i < a ) {
		item_a = item_a->next;
		i++;
	}
	item_b = item_a->next;

	if(b == list->count - 1) {		//b가 list의 마지막 entry인 경우
		item_a->next = NULL;
		item_b->next = item_a;
		list->last = item_a;
	}

	else {
		item_a->next = item_b->next;
		item_b->next = item_a;
	}

	if(a == 0) {					//a가 list의 첫 번째 entry인 경우 
		list->first = item_b;
	}

	else {
		item = list->first;
		i = 0;
		while(i < a-1) {
			item = item->next;
			i++;
		}
		item->next = item_b;
	}

	return EXT2_SUCCESS;
}

int alpha_sort(SHELL_ENTRY_LIST* list)
{	
	SHELL_ENTRY_LIST_ITEM* item;

	if(list->count == 0) return EXT2_SUCCESS;

	for(int i = 0; i < list->count - 1; i++) {
		item = list->first;
		for(int j = 0; j < (list->count -1) - i; j++) {
			if(strcmp((const char *)item->entry.name, (const char *)item->next->entry.name) > 0)
				swap(list, j, j+1); 
			else item = item->next;
		}
	}

	return EXT2_SUCCESS;

}

int r_alpha_sort(SHELL_ENTRY_LIST* list)
{	
	SHELL_ENTRY_LIST_ITEM* item;

	if(list->count == 0) return EXT2_SUCCESS;

	for(int i = 0; i < list->count - 1; i++) {
		item = list->first;
		for(int j = 0; j < (list->count -1) - i; j++) {
			if(strcmp((const char*)item->entry.name, (const char*)item->next->entry.name) < 0)
				swap(list, j, j+1); 
			else item = item->next;
		}
	}

	return EXT2_SUCCESS;

}

int size_sort(SHELL_ENTRY_LIST* list) 
{
	SHELL_ENTRY_LIST_ITEM* item;

	if(list->count == 0) return EXT2_SUCCESS;

	for(int i = 0; i < list->count - 1; i++) {
		item = list->first;
		for(int j = 0; j < (list->count -1) - i; j++) {
			if(item->entry.size > item->next->entry.size)
				swap(list, j, j+1); 
			else item = item->next;
		}
	}

	return EXT2_SUCCESS;
}
