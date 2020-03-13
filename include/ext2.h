#ifndef _FAT_H_
#define _FAT_H_

#include "common.h"
#include "disk.h"
#include "string.h"

//#include <sys/resource.h>
//#include <sys/types.h>
//#include <unistd.h>
//#include <time.h>

#define     EXT2_N_BLOCKS           15
#define		NUMBER_OF_SECTORS		( 240 + 2 )		//( 8192 + 2 )	//sector 크기 512 byte
#define		NUMBER_OF_GROUPS		4
#define		NUMBER_OF_INODES		700
#define		VOLUME_LABLE			"NC 19th"

#define MAX_SECTOR_SIZE			512
#define MAX_BLOCK_SIZE			1024
#define MAX_NAME_LENGTH			256
#define MAX_ENTRY_NAME_LENGTH	11
#define MAX_BLOCK_PER_FILE		140000

#define GDT_BLOCK_COUNT 		2		//group descriptor table의 블록 개수
#define INODETABLE_BLOCK_COUNT 	4		//inode table의 블록 개수

#define UID 0x1234
#define GID 0x1234

#define ATTR_READ_ONLY			0x01
#define ATTR_HIDDEN				0x02
#define ATTR_SYSTEM				0x04
#define ATTR_VOLUME_ID			0x08
#define ATTR_DIRECTORY			0x10
#define ATTR_ARCHIVE			0x20
#define ATTR_LONG_NAME			ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID

#define DIR_ENTRY_NO_MORE		0x00
#define DIR_ENTRY_OVERWRITE		1

#define GET_INODE_GROUP(x) ((x) - 1)/( NUMBER_OF_INODES / NUMBER_OF_GROUPS )

#ifdef _WIN32
#pragma pack(push,fatstructures)
#endif
#pragma pack(1)

typedef struct
{
	UINT32 inode_count;				//�ִ� inode����
	UINT32 block_count;				//���� �ý��� ��ü block����
	UINT32 reserved_block_count;	
	UINT32 free_block_count;	
	UINT32 free_inode_count;

	UINT32 first_data_block;		//block group0�� ���۵Ǵ� ����
	UINT32 log_block_size;			//block size�� 2^n * 1024 byte�� �� n��
	UINT32 log_fragmentation_size;
	UINT32 block_per_group;			
	UINT32 fragmentation_per_group;
	UINT32 inode_per_group;
	int mtime;
	//time_t mtime;					//���������� ���Ͻý����� mount�� �ð�
	//UINT32 wtime;					//���������� superblock�� ������ �ð�
	//UINT16 mount_count
	//UINT16 max_mount_count
	UINT16 magic_signature;
	UINT16 state;					//���Ͻý����� ���� 
	/* EXT2_VALID_FS 0x0001		������ ���
	   EXT2_ERROR_FS 0x0002		error �߻�   */
	UINT16 errors;
	/* EXT2_ERRORS_CONTINUE	0	����
	   EXT2_ERRORS_RO		1	READ ONLY
	   EXT2_ERRORS_PANIC	2	�дл���
	   EXT2_ERRORS_DEFAULT	0	�⺻��(����) */
	//UINT16 minor_version
	UINT32 creator_os;			//EXT2_OS_LINUX 0
	UINT32 first_non_reserved_inode;	//������� ���� inode�� ù ��° �ε���
	UINT16 inode_structure_size;		//inode ����ü�� ũ�� 

	UINT16 block_group_number;			//������ superblock�� �����ϰ� �ִ� block group
	UINT32 first_data_block_each_group; //�� �׷쳻���� ù ��° datablock�� ��ȣ 

	BYTE volume_name[16];

} EXT2_SUPER_BLOCK;

typedef struct
{
	UINT16  mode;         /* File mode */
	UINT16  uid;          /* Low 16 bits of Owner Uid */
	UINT32  size;         /* Size in bytes */
	UINT32  atime;        /* Access time */
	UINT32  ctime;        /* Creation time */
	UINT32  mtime;        /* Modification time */
	UINT32  dtime;        /* Deletion Time */
	UINT16  gid;          /* Low 16 bits of Group Id */
	UINT16  links_count;  /* Links count */
	UINT32  blocks;       /* Blocks count */
	UINT32  flags;        /* File flags */
	UINT32  i_reserved1;   // OS dependent 1
	UINT32  block[EXT2_N_BLOCKS]; /* Pointers to blocks */
	UINT32  generation;   /* File version (for NFS) */
	UINT32  file_acl;     /* File ACL */
	UINT32  dir_acl;      /* Directory ACL */
	UINT32  faddr;        /* Fragment address */
	UINT32  i_reserved2[3];   // OS dependent 2
} INODE;

typedef struct
{
	UINT32 start_block_of_block_bitmap;
	UINT32 start_block_of_inode_bitmap;
	UINT32 start_block_of_inode_table;
	UINT32 free_blocks_count;
	UINT32 free_inodes_count;
	UINT16 directories_count;
	BYTE padding[2];
	BYTE reserved[12];
} EXT2_GROUP_DESCRIPTOR;

typedef struct
{
	UINT32 inode;
	BYTE name[11];
	UINT32 name_len;
	BYTE pad[13];
} EXT2_DIR_ENTRY;

#ifdef _WIN32
#pragma pack(pop, fatstructures)
#else
#pragma pack()
#endif

typedef struct
{
	EXT2_SUPER_BLOCK		sb;
	EXT2_GROUP_DESCRIPTOR	gd;
	DISK_OPERATIONS*		disk;

} EXT2_FILESYSTEM;

typedef struct
{
	UINT32 group;
	UINT32 block;
	UINT32 offset;
} EXT2_DIR_ENTRY_LOCATION;

typedef struct
{
	EXT2_FILESYSTEM * fs;
	EXT2_DIR_ENTRY entry;
	EXT2_DIR_ENTRY_LOCATION location;

} EXT2_NODE;

#define SUPER_BLOCK 0
#define GROUP_DES  1
#define BLOCK_BITMAP GDT_BLOCK_COUNT + 1
#define INODE_BITMAP GDT_BLOCK_COUNT + 2
#define INODE_TABLE(x) (GDT_BLOCK_COUNT + 3 + x)

#define FILE_TYPE_FIFO               0x1000
#define FILE_TYPE_CHARACTERDEVICE    0x2000
#define FILE_TYPE_DIR				 0x4000
#define FILE_TYPE_BLOCKDEVICE        0x6000
#define FILE_TYPE_FILE				 0x8000

typedef int (* EXT2_NODE_ADD)(EXT2_FILESYSTEM *, void *, EXT2_NODE *);

int ext2_format(DISK_OPERATIONS* disk);
int ext2_create(EXT2_NODE* parent, char* entryName, EXT2_NODE* retEntry);
int ext2_write(EXT2_NODE* file, unsigned long offset, unsigned long length, const char* buffer);
int ext2_lookup(EXT2_NODE* parent, const char* entryName, EXT2_NODE* retEntry);
int ext2_mkdir(const EXT2_NODE* parent, const char* entryName, EXT2_NODE* retEntry);
int ext2_read(EXT2_NODE* file, unsigned long offset, unsigned long length, char* buffer);
int sector_read(DISK_OPERATIONS* disk, SECTOR group, SECTOR block, SECTOR sector_num, BYTE* data);

int block_write(DISK_OPERATIONS* disk, SECTOR group, SECTOR block, BYTE* data);
int sector_write(DISK_OPERATIONS* disk, SECTOR group, SECTOR block, SECTOR sector_num, BYTE* data);
int block_read(DISK_OPERATIONS* disk, SECTOR group, SECTOR block, BYTE* data);
int ext2_read_dir(EXT2_NODE* dir, EXT2_NODE_ADD adder, void* list);
int read_superblock(EXT2_FILESYSTEM* fs);
int fill_node_struct(EXT2_FILESYSTEM* fs, EXT2_NODE* root);
int get_inode(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK* sb, const UINT32 inode, INODE *inodeBuffer);
int set_inode(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK* sb, const UINT32 inode, INODE *inodeBuffer);
int insert_entry(EXT2_FILESYSTEM* fs, const EXT2_NODE* parent, EXT2_DIR_ENTRY* entry,  EXT2_DIR_ENTRY_LOCATION* location);
int get_data_block_at_inode(EXT2_FILESYSTEM *fs, INODE* inode, UINT32 number, UINT32* block);
int set_block_bitmap(DISK_OPERATIONS* disk, SECTOR group, SECTOR block_num, int number);
int set_inode_bitmap(DISK_OPERATIONS* disk, SECTOR inode_num, int number);
int set_inode_block(EXT2_FILESYSTEM* fs, UINT32 i_num, UINT32 b_num);
int get_inode_block(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK* sb, UINT32 i_num, UINT32 b_num);
int alloc_free_block(const EXT2_FILESYSTEM* fs, UINT32 group);
int alloc_free_inode(EXT2_FILESYSTEM * fs, EXT2_NODE * parent);
int fill_super_block(EXT2_SUPER_BLOCK * sb);
int fill_descriptor_block(EXT2_GROUP_DESCRIPTOR * gd, EXT2_SUPER_BLOCK * sb);
int release_dir_entry(EXT2_FILESYSTEM* fs, EXT2_NODE* parent, EXT2_NODE* entry);
int release_inode(EXT2_FILESYSTEM* fs, EXT2_NODE* entry);
int create_root(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK * sb);
int release_block(EXT2_FILESYSTEM* fs, INODE* inode, UINT32* b_num, int release_num, UINT32* group);
int increase_free_block(EXT2_FILESYSTEM* fs, UINT32* group);
#endif

