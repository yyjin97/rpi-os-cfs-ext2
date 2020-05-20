typedef struct
{
	char*	address;
} DISK_MEMORY;

#include "ext2.h"
#define MIN( a, b )					( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b )					( ( a ) > ( b ) ? ( a ) : ( b ) )

int ext2_write(EXT2_NODE* file, unsigned long offset, unsigned long length, const char* buffer)
{
	UINT32 end_write;
	UINT32 file_inode_num;
	UINT32 offset_block;
	UINT32 block;
	UINT32 beginoffset;
	UINT32 wb_num[MAX_BLOCK_PER_FILE];//write block num
    INT32 g_num;
	INT32 g=0;
	INT32 block_num;
	UINT32 endoffset;
	UINT32 blockPerGroup = file->fs->sb.block_per_group;
	BYTE blockBuffer[MAX_BLOCK_SIZE];

	INODE inode;

	file_inode_num = file->entry.inode;
	get_inode(file->fs->disk,&file->fs->sb,file_inode_num, &inode);

	if(inode.size < offset) {
		printf("offset value has exceeded the file size\n\r");
		return EXT2_ERROR;
	}

    offset_block = offset / MAX_BLOCK_SIZE;//offset이차지하는 block 개수
	beginoffset = offset % MAX_BLOCK_SIZE;//offset의 나머지
	end_write = beginoffset + length;
    endoffset = end_write % MAX_BLOCK_SIZE;
	block = ( offset + length ) / MAX_BLOCK_SIZE;

	if(inode.blocks>=block)//inode에 할당 해야할 블록보다 더많이 있거나 같을때
	{
		if((endoffset>0)&&(inode.blocks==block)){ block=1;}/* 만약 할당해야할 블록과 inode안에 블록개수가 같으면
		                                                       curroffset_end가 0이냐 아니냐에 따라 다르다 */
		else{block = 0;}
	}
	else
	{
		if(endoffset>0) block = block - inode.blocks + 1;
	    else  block = block - inode.blocks;
	}

	if(inode.size<(offset+length))
	{
		inode.size = offset+length;
	}

    set_inode(file->fs->disk,&file->fs->sb,file_inode_num,&inode);
	get_inode(file->fs->disk,&file->fs->sb,file_inode_num, &inode);

    /* offset + length가  차지하는 block들을 할당 */
    ZeroMemory(wb_num, sizeof(wb_num));
	if(block>0 )
	{
		
        g_num =  GET_INODE_GROUP(file_inode_num);
		g=g_num;
    
		for(int i=0; i<block; i++)
		{
		
			do{
		
			block_num = alloc_free_block(file->fs, g);		//새로운 block을 할당
            
			if(block_num < 0) return EXT2_ERROR;
			else if(block_num == 0) g = (g+1)%NUMBER_OF_GROUPS;	//해당 group에 free block이 존재하지 않는 경우 
			else									//해당 group에서 free block을 찾은 경우 
			{
				g_num = g;
			    break;
			}
		    
			} while(g != g_num);

			if(block_num == 0) {
				printf("No more free block\n\r");
				return EXT2_ERROR;
			}
			if(set_inode_block(file->fs,file_inode_num,g_num*file->fs->sb.block_per_group + block_num)) return EXT2_ERROR;
            

		}
    }
    /* 파일의 inode 로부터 할당한 블럭들을 모두 가져온다 */
    get_data_block_at_inode(file->fs,&inode,file_inode_num,wb_num);

    /* 내용이 담겨있는 첫번째 블록을 읽어온다*/
	ZeroMemory(blockBuffer, sizeof(blockBuffer));
	block_num = wb_num[offset_block];


	if(beginoffset>0)// 한블럭내에서 시작지점의 오프셋이 0보다 클때
	{

      block_read(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);

	 /* 쓰고자 하는 길이가 블럭 사이즈에서 오프셋을 제외한 길이 보다 작을 때 */
	  if(length<(MAX_BLOCK_SIZE-beginoffset)) {
		memcpy((unsigned long)blockBuffer+beginoffset,(unsigned long)buffer,(unsigned long)length);

	  block_write(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);
	  return EXT2_SUCCESS;
      }

	  /* 쓰고자 하는 길이가 블럭사이즈에서 오프셋을 제외한 길이보다 크거나 같을 때  */
	  memcpy((unsigned long)(blockBuffer+beginoffset),(unsigned long)buffer,(unsigned long)(MAX_BLOCK_SIZE-beginoffset));
	  block_write(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);	

	  buffer=buffer+MAX_BLOCK_SIZE-beginoffset;
    }
	
	else //블럭의 맨처음부터 시작할때
	{

	  ZeroMemory(blockBuffer, sizeof(blockBuffer));
	  block_read(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);
      /* 쓰고자 하는 크기(length)가 블록 크기보다 작을때 */
	  if(length<MAX_BLOCK_SIZE){
	    memcpy((unsigned long)blockBuffer,(unsigned long)buffer,(unsigned long)length);
		block_write(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);
		return EXT2_SUCCESS;
	  }
     /* 쓰고자 하는 크기(length)가 블록 크기보다 크거나 같을 때 */
	  memcpy((unsigned long)blockBuffer,(unsigned long)buffer,(unsigned long)MAX_BLOCK_SIZE);
 	  block_write(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);
     
	  if(length==MAX_BLOCK_SIZE){return EXT2_SUCCESS;}
	  buffer=buffer+MAX_BLOCK_SIZE;
	}
	
	block=end_write/MAX_BLOCK_SIZE;//처음블럭과 마지막 블럭을 제외하고 쓰는데 필요한 블럭개수
	endoffset = (end_write)%MAX_BLOCK_SIZE;// 마지막지점의 오프셋

    /*  블록크기만큼 읽어서 write */
    for(int i=1; i<block;i++)
	{
		ZeroMemory(blockBuffer, sizeof(blockBuffer));
		block_num=wb_num[offset_block+i];
	
		memcpy((unsigned long)blockBuffer,(unsigned long)buffer,(unsigned long)MAX_BLOCK_SIZE);
	  
		block_write(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);
		buffer=buffer+MAX_BLOCK_SIZE;
    }

    /* 마지막에 endoffset있게 되면 그 크기만큼 write */
	if(endoffset>0&&block>0)
	{
	  ZeroMemory(blockBuffer, sizeof(blockBuffer));
      block_num = wb_num[offset_block+block];

      memcpy((unsigned long)blockBuffer,(unsigned long)buffer,(unsigned long)endoffset);
	  
	  block_write(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);
	}
	 
	 return EXT2_SUCCESS;
}

int get_block_group( EXT2_FILESYSTEM*fs, UINT32 block_num)
{
	UINT32 group;
	UINT32 blocks_per_group = fs->sb.block_count/NUMBER_OF_GROUPS;

	group = block_num/(blocks_per_group);//그룹당 수퍼블록의 개수를 구하고 그것을 나눈다 
	
	/* 맨끝에 있는 그룹은 블록개수가 blocks_per_group 보다 많을 수도 있다는걸 고려해서 group 반환  */
	if(group == NUMBER_OF_GROUPS && ((fs->sb.block_count%NUMBER_OF_GROUPS)<blocks_per_group))
	{ group = NUMBER_OF_GROUPS-1; return group;}
	else if(group<NUMBER_OF_GROUPS) return group;
	else return EXT2_ERROR;
}

/* file의 내용을 offset부터 length만큼 buffer에 read해오는 함수 */
int ext2_read(EXT2_NODE* file, unsigned long offset, unsigned long length, char* buffer)
{
	INODE inode;
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 block_num[MAX_BLOCK_PER_FILE];
	UINT32 g_num;
	UINT32 b_num;
	UINT32 b_offset;
	UINT32 b;
	UINT32 blockPerGroup = file->fs->sb.block_per_group;

	get_data_block_at_inode(file->fs, &inode, file->entry.inode, block_num);

	b_offset = offset%MAX_BLOCK_SIZE;		//'offset'의 block내 offset값
	b = offset/MAX_BLOCK_SIZE;				//'offset'의 block 번호 

	if(offset > inode.size) {				//offset이 file의 size보다 큰 경우 error발생 
		printf("offset has exceeded the file size\n\r");
		return -1;
	}

	if(b_offset+length < MAX_BLOCK_SIZE) {	//read하는 내용이 block의 범위를 넘어가지 않는 경우 
		ZeroMemory(block, sizeof(block));
		g_num = block_num[b]/blockPerGroup;
		b_num = block_num[b]%blockPerGroup;
		block_read(file->fs->disk, g_num, b_num, block);
		memcpy((unsigned long)buffer, (unsigned long)block+b_offset, (unsigned long)length);
		return EXT2_SUCCESS;
	}

	else {
		ZeroMemory(block, sizeof(block));
		g_num = block_num[b]/blockPerGroup;
		b_num = block_num[b]%blockPerGroup;
		block_read(file->fs->disk, g_num, b_num, block);			//offset이 존재하는 block을 read해옴
		memcpy((unsigned long)buffer, (unsigned long)(block+b_offset), (unsigned long)MAX_BLOCK_SIZE-b_offset);	//offset이후부터 block단위까지 read함
		buffer += (MAX_BLOCK_SIZE-b_offset);
		b++;

		for(int i = 0; i < (length-(MAX_BLOCK_SIZE-b_offset))/MAX_BLOCK_SIZE; i++) {	//block 전체를 read해오는 경우 
			ZeroMemory(block, sizeof(block));
			g_num = block_num[b]/blockPerGroup;
			b_num = block_num[b]%blockPerGroup;
			block_read(file->fs->disk, g_num, b_num, block);
			memcpy((unsigned long)buffer, (unsigned long)block, (unsigned long)MAX_BLOCK_SIZE);
			b++;
			buffer += MAX_BLOCK_SIZE;
		}

		if((length-(MAX_BLOCK_SIZE-b_offset))%MAX_BLOCK_SIZE == 0) return EXT2_SUCCESS;		//read할 내용이 block의 단위에 맞는 경우 (더이상 read할 블록이 없는 경우)
		else {										//read할 내용이 남은 경우 
			ZeroMemory(block, sizeof(block));
			g_num = block_num[b]/blockPerGroup;
			b_num = block_num[b]%blockPerGroup;
			block_read(file->fs->disk, g_num, b_num, block);
			memcpy((unsigned long)buffer, (unsigned long)block, (unsigned long)((length-(MAX_BLOCK_SIZE-b_offset))%MAX_BLOCK_SIZE));
		}
		
	}

	return EXT2_SUCCESS;

}

int ext2_format(DISK_OPERATIONS* disk)
{
	EXT2_SUPER_BLOCK sb;
	EXT2_GROUP_DESCRIPTOR gd;

	BYTE block[MAX_BLOCK_SIZE];
	UINT32 blockInUse;

	/* super block 초기화하고 disk에 write */
	if (fill_super_block(&sb) != EXT2_SUCCESS)
		return EXT2_ERROR;

	for(int g_num = 0; g_num < NUMBER_OF_GROUPS; g_num++ ){		//모든 block group에 대해 super block을 write
		ZeroMemory(block, sizeof(block));
		memcpy((unsigned long)block, (unsigned long)&sb, (unsigned long)sizeof(EXT2_SUPER_BLOCK));
		block_write(disk, g_num, 0, block);
		sb.block_group_number = g_num;
	}

	/* group descriptor table 초기화하고 disk에 write */
	if (fill_descriptor_block(&gd, &sb) != EXT2_SUCCESS)
		return EXT2_ERROR;

	for(int i = 0;i<GDT_BLOCK_COUNT;i++) {			//group descriptor table의 block개수만큼 반복
		static int g_num = 0;
		ZeroMemory(block, sizeof(block));			//block을 0으로 초기화 
		for(; g_num < NUMBER_OF_GROUPS; g_num++) {	//모든 group의 group descriptor 구조체를 block에 복사
			if((g_num+1)*sizeof(EXT2_GROUP_DESCRIPTOR) > (i+1)*MAX_BLOCK_SIZE) break;

			if(g_num == 1) gd.free_inodes_count += 10;		//block group0만 예약된 inode 10개 제외, group1부터는 10개 추가

			if(g_num == NUMBER_OF_GROUPS - 1) {				//마지막 block group인 경우 
				gd.free_inodes_count += (UINT16)(NUMBER_OF_INODES % NUMBER_OF_GROUPS);		
				gd.free_blocks_count += (UINT16)(sb.free_block_count % NUMBER_OF_GROUPS);
			}

			memcpy((unsigned long)(block + g_num*sizeof(EXT2_GROUP_DESCRIPTOR)), (unsigned long)&gd, (unsigned long)sizeof(EXT2_GROUP_DESCRIPTOR));
		}

		for(int gn = 0; gn < NUMBER_OF_GROUPS; gn++) {		//group descriptor table의 블록을 모든 group의 disk에 write
			block_write(disk, gn, 1+i, block);
		}
	}

	/* data block bitmap 초기화하고 disk에 write */
	ZeroMemory(block, sizeof(block));
	blockInUse = 1 + GDT_BLOCK_COUNT + 1 + 1 + INODETABLE_BLOCK_COUNT;	//사용중인 block개수 
	int i = 0;
	for(; i < blockInUse/8; i++) {
		block[i] = 0xff;
	}
	block[i] |=  (0xff >> (8 - blockInUse%8));

	for(int gn = 0; gn < NUMBER_OF_GROUPS; gn++) {		//block bitmap의 블록을 모든 group의 disk에 write
		block_write(disk, gn, gd.start_block_of_block_bitmap, block);
	}

	/* inode bitmap 초기화하고 disk에 write */
	ZeroMemory(block, sizeof(block));
	block[0] = 0xff;
	block[1] = 0x03;

	for(int gn = 0; gn < NUMBER_OF_GROUPS; gn++) {		//inode bitmap의 블록을 모든 group의 disk에 write
		if(gn == 1) ZeroMemory(block, sizeof(block));
		block_write(disk, gn, gd.start_block_of_inode_bitmap, block);
	}

	/* inode table 초기화하고 disk에 write */
	ZeroMemory(block, sizeof(block));
	for(int gn = 0; gn < NUMBER_OF_GROUPS; gn++) {
		for(int bn = 0; bn < INODETABLE_BLOCK_COUNT; bn++){
			block_write(disk, gn, gd.start_block_of_inode_table + bn, block);
		}
	}

	/* format 정보 출력 */
	printf("inode count                     : %u\n\r", sb.inode_count);
	printf("inode per group                 : %u\n\r", sb.inode_per_group);
	printf("inode size                      : %u\n\r", sb.inode_structure_size);
	printf("block count                     : %u\n\r", sb.block_count);
	printf("block per group                 : %u\n\r", sb.block_per_group);
	printf("first data block each group     : %u\n\r", sb.first_data_block_each_group);
	printf("block byte size                 : %u\n\r", MAX_BLOCK_SIZE);
	printf("sector count                    : %u\n\r", NUMBER_OF_SECTORS);
	printf("sector byte size                : %u\n\n\r", MAX_SECTOR_SIZE);

	create_root(disk, &sb);

	return EXT2_SUCCESS;
}

int block_write(DISK_OPERATIONS* disk, SECTOR group, SECTOR block, BYTE* data)  //'group'번째 그룹의 'block'번째 블록에 data의 내용을 write
{
	SECTOR sectorPerBlock = MAX_BLOCK_SIZE >> 9;
	BYTE sector[MAX_SECTOR_SIZE];

	for(int s_num = 0; s_num < sectorPerBlock; s_num++){
		ZeroMemory(sector, sizeof(sector));
		memcpy((unsigned long)sector, (unsigned long)(data + (MAX_SECTOR_SIZE*s_num)), (unsigned long)sizeof(sector));
		sector_write(disk, group, block, s_num, sector);			//sector 단위로 read하는 함수
	}

	return EXT2_SUCCESS;

}

int sector_write(DISK_OPERATIONS* disk, SECTOR group, SECTOR block, SECTOR sector_num, BYTE* data)	//'group'번째 그룹 'block'번째 블록 'sector_num'번째 sector내용을 data변수에 읽어옴
{
	SECTOR realsector;
	SECTOR sectorPerGroup = (NUMBER_OF_SECTORS - 2) / NUMBER_OF_GROUPS;
	SECTOR sectorPerBlock = MAX_BLOCK_SIZE >> 9;
	BYTE sector[MAX_SECTOR_SIZE];

	realsector = 2 + group*sectorPerGroup + block*sectorPerBlock + sector_num;		//해당 sector의 파일시스템 전체에서 sector변호를 구함

	ZeroMemory(sector, sizeof(sector));
	memcpy((unsigned long)sector, (unsigned long)data, (unsigned long)sizeof(sector));
	disk->write_sector(disk, realsector, sector);

	return EXT2_SUCCESS;
}

int block_read(DISK_OPERATIONS* disk, SECTOR group, SECTOR block, BYTE* data) 	//'group'번째 그룹의 'block'번째 블록을 'data'변수에 읽어옴
{
	SECTOR sectorPerBlock = MAX_BLOCK_SIZE >> 9;
	BYTE sector[MAX_SECTOR_SIZE];

	for(int s_num = 0; s_num < sectorPerBlock; s_num++){
		ZeroMemory(sector, sizeof(sector));
		sector_read(disk, group, block, s_num, sector);					//sector 단위로 read하는 함수
		memcpy((unsigned long)(data + (MAX_SECTOR_SIZE*s_num)), (unsigned long)sector, (unsigned long)sizeof(sector));
	}
	return EXT2_SUCCESS;
}

int sector_read(DISK_OPERATIONS* disk, SECTOR group, SECTOR block, SECTOR sector_num, BYTE* data)	//'group'번째 그룹 'block'번째 블록 'sector_num'번째 sector내용을 data변수에 읽어옴
{
	SECTOR realsector;
	SECTOR sectorPerGroup = (NUMBER_OF_SECTORS - 2) / NUMBER_OF_GROUPS;
	SECTOR sectorPerBlock = MAX_BLOCK_SIZE >> 9;
	BYTE sector[MAX_SECTOR_SIZE];

	realsector = 2 + group*sectorPerGroup + block*sectorPerBlock + sector_num;		//해당 sector의 파일시스템 전체에서 sector변호를 구함

	ZeroMemory(sector,sizeof(sector));
	disk->read_sector(disk, realsector, sector);		
	memcpy((unsigned long)data, (unsigned long)sector, (unsigned long)sizeof(sector));

	return EXT2_SUCCESS;
}

int fill_super_block(EXT2_SUPER_BLOCK * sb)	
{
	ZeroMemory(sb, sizeof(EXT2_SUPER_BLOCK));

	sb->inode_count = NUMBER_OF_INODES;   //700
	if(MAX_BLOCK_SIZE == 1024)	{
		sb->block_count = (NUMBER_OF_SECTORS - 2) / 2; 
		sb->log_block_size = 0;				//2^0 * 1KB = 1KB 
	}
	else if(MAX_BLOCK_SIZE == 2048)	{
		sb->block_count = (NUMBER_OF_SECTORS - 2) / 4;
		sb->log_block_size = 1;
	}
	else if(MAX_BLOCK_SIZE == 4096) {
		sb->block_count = (NUMBER_OF_SECTORS - 2) / 8;
		sb->log_block_size = 2;
	}
	else {
		printf("block size error\n\r");
		return EXT2_ERROR;
	} 
	sb->reserved_block_count = 0;
	sb->free_block_count = sb->block_count - ((1 + GDT_BLOCK_COUNT + 1 + 1 + INODETABLE_BLOCK_COUNT) * NUMBER_OF_GROUPS); //각 block group마다 super block, group descriptor, bitmap, inode table 제외
	sb->free_inode_count = NUMBER_OF_INODES - 10;      //inode 1번부터 10번까지는 예약됨
	sb->first_data_block = 0;         	
	
	sb->log_fragmentation_size = 0;		
	sb->block_per_group = sb->block_count / NUMBER_OF_GROUPS;  //sector개수에서 boot block을 제외하고 group개수로 나눔
	sb->fragmentation_per_group = 0;
	sb->inode_per_group = NUMBER_OF_INODES / NUMBER_OF_GROUPS;	

	sb->magic_signature = 0xEF53;		
	sb->state = 0x0001;
	sb->errors = 0;
	sb->creator_os = 0;						//linux
	sb->first_non_reserved_inode = 11;		//inode 1번부터 10번까지는 예약됨
	sb->inode_structure_size = 128; 		//inode크기 = 128 byte
	sb->block_group_number = 0;				
	sb->first_data_block_each_group = 1 + GDT_BLOCK_COUNT + 1 + 1 + INODETABLE_BLOCK_COUNT; //superblock + group descriptor + data bitmap + inode bitmap + inode table

	strcpy((char *)&sb->volume_name, (char *)VOLUME_LABLE);
	return EXT2_SUCCESS;
}

int fill_node_struct(EXT2_FILESYSTEM* fs, EXT2_NODE* root)		//root directory의 EXT2_NODE 구조체를 초기화함
{
	INODE inode;
	EXT2_DIR_ENTRY* entry;
	EXT2_DIR_ENTRY_LOCATION location;
	UINT32 block_num[MAX_BLOCK_PER_FILE];
	BYTE block[MAX_BLOCK_SIZE];
	
	ZeroMemory(block, sizeof(block));
	get_data_block_at_inode(fs, &inode, 2, block_num);
	printf("root block : %u\n\r", block_num[0]);

	block_read(fs->disk, 0, block_num[0], block);		//root directory의 data block을 읽어옴

	entry = (EXT2_DIR_ENTRY *)block;
	memcpy((unsigned long)(&root->entry), (unsigned long)entry, (unsigned long)sizeof(EXT2_DIR_ENTRY));

	root->fs = fs;

	location.group = 0;				//root directory의 location 초기화 
	location.block = fs->sb.first_data_block_each_group;
	location.offset = 0;
	memcpy((unsigned long)(&root->location), (unsigned long)&location, (unsigned long)sizeof(EXT2_DIR_ENTRY_LOCATION));

	return EXT2_SUCCESS;
}

int fill_descriptor_block(EXT2_GROUP_DESCRIPTOR * gd, EXT2_SUPER_BLOCK * sb)  //block group0의 group descriptor 초기화
{
	ZeroMemory(gd, sizeof(EXT2_GROUP_DESCRIPTOR));		//group descriptor 구조체를 0으로 초기화 

	gd->start_block_of_block_bitmap = 1 + GDT_BLOCK_COUNT;    	
	gd->start_block_of_inode_bitmap = 1 + GDT_BLOCK_COUNT + 1;	
	gd->start_block_of_inode_table = 1 + GDT_BLOCK_COUNT + 1 + 1;
	gd->free_blocks_count = (UINT16)(sb->free_block_count / NUMBER_OF_GROUPS);	//block group내의 free block의 개수 
	gd->free_inodes_count = (UINT16)(((sb->free_inode_count) + 10) / NUMBER_OF_GROUPS - 10);  	//예약된 inode포함하여 group당 inode의 개수를 구함
	gd->directories_count = 0;		

	return EXT2_SUCCESS;
}

/* 'group'번호의 그룹 내의 'block_num'번 block을 bitmap에서 'number'로 set */
int set_block_bitmap(DISK_OPERATIONS* disk, SECTOR group, SECTOR block_num, int number) 
{
	int bn = 0;
	BYTE block[MAX_BLOCK_SIZE];

	ZeroMemory(block, sizeof(block));
	block_read(disk, group, 1+GDT_BLOCK_COUNT, block);

	bn = block_num/8;

	if(number == 1) block[bn] |= (0x01 << (block_num%8));
	else if(number == 0) block[bn] &= ~(0x01 << (block_num%8));
	else return EXT2_ERROR;

	block_write(disk, group, 1+GDT_BLOCK_COUNT, block);

	return EXT2_SUCCESS;
}

/* 'inode_num'의 inode에 해당하는 inodebitmap을 'number'로 set */
int set_inode_bitmap(DISK_OPERATIONS* disk, SECTOR inode_num, int number)	
{
	int inode = 0;
	UINT32 inode_group = GET_INODE_GROUP(inode_num);					//해당 inode의 group번호 
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 inodePerGroup =  NUMBER_OF_INODES/NUMBER_OF_GROUPS;

	ZeroMemory(block, sizeof(block));
	block_read(disk, inode_group, 1+GDT_BLOCK_COUNT+1, block);

	if(inode_num > inodePerGroup) {
		inode_num -= inode_group*inodePerGroup;
	}
	inode = (inode_num - 1)/8;

	if(number == 1) block[inode] |= (0x01 << ((inode_num-1)%8));
	else if(number == 0) block[inode] &= ~(0x01 << ((inode_num-1)%8));
	else return EXT2_ERROR;

	block_write(disk, inode_group, 1+GDT_BLOCK_COUNT+1, block);

	return EXT2_SUCCESS;
}

int create_root(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK * sb)
{
	EXT2_DIR_ENTRY* entry;
	EXT2_GROUP_DESCRIPTOR* gd;
	INODE inode;
	BYTE block[MAX_BLOCK_SIZE];
	char* dot = ".";
	char* dotdot = "..";

	ZeroMemory(block, sizeof(block));

	entry = (EXT2_DIR_ENTRY *)block;

	/* dot entry */
	entry->inode = 2; 	//root directory의 inode번호 2번
	strcpy((char *)&entry->name, (char *)dot);
	entry->name_len = strlen(dot);

	entry++;
	/* dotdot entry */
	entry->inode = 2;
	strcpy((char *)&entry->name, (char *)dotdot);
	entry->name_len = strlen(dotdot);

	entry++;
	entry->name[0] = DIR_ENTRY_NO_MORE;

	block_write(disk, 0, sb->first_data_block_each_group, block);  	//root directory의 data block을 disk에 write
	printf("group: %d block: %d\n\r",0,sb->first_data_block_each_group);

	set_block_bitmap(disk, 0, sb->first_data_block_each_group, 1);		//block bitmap 수정

	/* inode 할당하고 write */
	ZeroMemory(block, sizeof(block));
	inode.mode = 0x01a4 | 0x4000;
	inode.uid = UID;
	inode.size = 2 * sizeof(EXT2_DIR_ENTRY);
	inode.gid = GID;
	inode.blocks = 1;					
	inode.block[0] = sb->first_data_block_each_group;  
	inode.links_count = 2;

	set_inode(disk, sb, 2, &inode);
	set_inode_bitmap(disk, 2, 1);


	/* group descriptor 수정 */
	ZeroMemory(block, sizeof(block));
	block_read(disk, 0, 1, block);
	gd = (EXT2_GROUP_DESCRIPTOR *)block;
	gd->free_blocks_count -= 1;
	gd->directories_count = 1;
	block_write(disk, 0, 1, block);

	/* super block 수정 */
	ZeroMemory(block, sizeof(block));
	memcpy((unsigned long)block, (unsigned long)sb, (unsigned long)sizeof(EXT2_SUPER_BLOCK));
	sb->free_block_count -= 1;
	block_write(disk, 0, 0, block);

	return EXT2_SUCCESS;
}

/* 'inode'번호에 해당하는 inode의 내용을 inodeBuffer에 읽어옴 */
int get_inode(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK* sb, const UINT32 inode, INODE *inodeBuffer)	
{	
	INODE *inode_read;
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 inode_group = (inode-1) / sb->inode_per_group;	//'inode'가 몇 번째 그룹에 속하는지 구함
	UINT32 inode_per_block = MAX_BLOCK_SIZE / sb->inode_structure_size;	//한 block에 몇 개의 inode가 있는지 구함
	UINT32 inodeIngroup = (inode-1) % sb->inode_per_group;			//group내에서 몇 번째 inode인지 구함
	UINT32 inode_block;
	UINT32 inode_num;

	inode_block = 1 + GDT_BLOCK_COUNT + 1 + 1 + (inodeIngroup / inode_per_block);	//'inode'가 그룹 내에서 몇 번째 block에 속하는지 구함 
	inode_num = (inode-1) % inode_per_block;	//block내에서 inode번호 
	ZeroMemory(block, sizeof(block));
	block_read(disk, inode_group, inode_block, block);

	inode_read = (INODE *)block;
	memcpy((unsigned long)inodeBuffer, (unsigned long)(inode_read + inode_num), (unsigned long)sizeof(INODE));

	return EXT2_SUCCESS;
}

/* 'inode'번호에 해당하는 inode위치에 'inodeBuffer'의 내용을 write함 */
int set_inode(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK* sb, const UINT32 inode, INODE *inodeBuffer)
{
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 inode_group = GET_INODE_GROUP(inode);	//'inode'가 몇 번째 그룹에 속하는지 구함
	UINT32 inode_per_block = MAX_BLOCK_SIZE / sb->inode_structure_size;	//한 block에 몇 개의 inode가 있는지 구함
	UINT32 inodeIngroup = (inode-1) % sb->inode_per_group;			//group내에서 몇 번째 inode인지 구함
	UINT32 inode_block;
	UINT32 inode_num;

	inode_block = 1 + GDT_BLOCK_COUNT + 1 + 1 + (inodeIngroup / inode_per_block);	//'inode'가 그룹 내에서 몇 번째 block에 속하는지 구함 
	inode_num = (inode-1) % inode_per_block;	//block내에서 inode번호 

	ZeroMemory(block, sizeof(block));
	block_read(disk, inode_group, inode_block, block);
	memcpy((unsigned long)(block + (inode_num * sizeof(INODE))), (unsigned long)inodeBuffer, (unsigned long)sizeof(INODE));

	block_write(disk, inode_group, inode_block, block);

	return EXT2_SUCCESS;
}

int ext2_create(EXT2_NODE* parent, const char* entryName, EXT2_NODE* retEntry) //파일이름을 받아서 그파일을 만든다 한파일당 사이즈 block 1개 
{
	if ((parent->fs->gd.free_inodes_count) == 0) return EXT2_ERROR;

	BYTE name[MAX_NAME_LENGTH] = { 0, };
    BYTE block[MAX_BLOCK_SIZE];
	INODE file_inode;
	INODE parent_inode;
	INT32 g_num , g;
	INT32 i_num;
	INT32 b_num;
    EXT2_SUPER_BLOCK* sb;
	EXT2_GROUP_DESCRIPTOR* gd;

	strcpy((char *)name, (char *)entryName);	//entry의 이름을 따온다 
	retEntry->entry.name_len = strlen((char *)name);
	retEntry->fs = parent->fs;

	if(ext2_lookup(parent, (const char *)name,retEntry) == EXT2_SUCCESS) {
		printf("%s file name already exists\n\r", entryName);
		return EXT2_ERROR;		//lookup 에서 해당 파일이름의 엔트리가 parent에 존재하는 지 검사
	}
	strcpy((char *)retEntry->entry.name, (char *)name);

	i_num = alloc_free_inode(retEntry->fs, parent);
	if(i_num < 0) {
		printf("alloc_free_inode error\n\r");
		return EXT2_ERROR;
	}

    g_num =  GET_INODE_GROUP(i_num);
	g = g_num;

    do{
		b_num = alloc_free_block(parent->fs, g);		//새로운 block을 할당
           
		if(b_num < 0) return EXT2_ERROR;
		else if(b_num == 0) {
			g++;
			g = g % NUMBER_OF_GROUPS;	//해당 group에 free block이 존재하지 않는 경우 
		}
		else  {									//해당 group에서 free block을 찾은 경우 
			g_num = g;
		    break;
		}	    
	} while(g != g_num);

	if(b_num == 0) {
		printf("No more free block\n\r");
		return EXT2_ERROR; 
	}
	
	//////ZeroMemory(block,sizeof(block));
	get_inode(parent->fs->disk, &parent->fs->sb,parent->entry.inode, &parent_inode);
	file_inode.mode = 0x1e4 | 0x8000;
	file_inode.uid = parent_inode.uid;
	file_inode.gid = parent_inode.gid;
	file_inode.blocks = 1;
	file_inode.block[0] = b_num;
	file_inode.size = 0;
	file_inode.links_count = 1;
	set_inode(parent->fs->disk,&parent->fs->sb,i_num,&file_inode);
	retEntry->entry.inode = i_num;
	
	if(insert_entry(parent->fs,parent,&retEntry->entry,&retEntry->location) == EXT2_ERROR)return EXT2_ERROR;

	/* superblock 수정 */
	ZeroMemory(block, sizeof(block));
	sb = (EXT2_SUPER_BLOCK *)block; 
	block_read(parent->fs->disk, 0, 0, block);
	sb->free_inode_count -= 1;
	block_write(parent->fs->disk, 0, 0, block);
	memcpy((unsigned long)&parent->fs->sb, (unsigned long)sb, (unsigned long)sizeof(EXT2_SUPER_BLOCK));
	memcpy((unsigned long)&retEntry->fs->sb, (unsigned long)sb, (unsigned long)sizeof(EXT2_SUPER_BLOCK));

	/* group descriptor 수정 */
	ZeroMemory(block, sizeof(block));
	gd = (EXT2_GROUP_DESCRIPTOR *)block;
	block_read(parent->fs->disk, 0, 1 + (g_num/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
	gd += (g_num % (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR)));
	gd->free_inodes_count -= 1;
	block_write(parent->fs->disk, 0, 1 + (g_num/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
	memcpy((unsigned long)&retEntry->fs->gd, (unsigned long)gd, (unsigned long)sizeof(EXT2_GROUP_DESCRIPTOR));

	return EXT2_SUCCESS;
}

/* 'inode'변수에 inode 구조체를 읽어오고 block' 배열에 data block의 번호 저장 */
int get_data_block_at_inode(EXT2_FILESYSTEM* fs, INODE* inode, UINT32 number, UINT32* block) 	
{
	if(get_inode(fs->disk, &fs->sb, number, inode) != EXT2_SUCCESS)
		return EXT2_ERROR;

	for(int i = 0; i < inode->blocks; i++) {
		block[i] = get_inode_block(fs->disk, &fs->sb, number, i);
	}

	return EXT2_SUCCESS;
}

/* superblock과 group descriptor를 읽어와 file system에 복사 */
int read_superblock(EXT2_FILESYSTEM* fs)
{
	BYTE sector[MAX_SECTOR_SIZE];

	if (fs == NULL || fs->disk == NULL)
	{
		printf("No file system struct\n\r");
		return EXT2_ERROR;
	}

	ZeroMemory(sector, sizeof(sector));
	sector_read(fs->disk, 0, SUPER_BLOCK, 0, sector);		//superblock을 disk에서 읽어옴 
	memcpy((unsigned long)&fs->sb, (unsigned long)sector, (unsigned long)sizeof(EXT2_SUPER_BLOCK));

	if(fs->sb.magic_signature != 0xEF53)					//super block의 magic number를 확인 
		return EXT2_ERROR;

	ZeroMemory(sector, sizeof(sector));
	sector_read(fs->disk, 0, GROUP_DES, 0, sector);				//group descriptor를 disk에서 읽어옴 
	memcpy((unsigned long)&fs->gd, (unsigned long)sector, (unsigned long)sizeof(EXT2_GROUP_DESCRIPTOR));		//0번 group의 group descriptor를 file system에 저장

	return EXT2_SUCCESS;
}

//해당이름의 파일 혹은 디렉토리가 존재하는지 확인하고 EXT2_NODE구조체를 가져온다 
int ext2_lookup(EXT2_NODE* parent, const char* entryName, EXT2_NODE* retEntry)
{
	UINT32 parent_inode;
	BYTE block[MAX_BLOCK_SIZE];
    UINT32 block_num[MAX_BLOCK_PER_FILE];	
	INODE inode;
	EXT2_DIR_ENTRY* entry;
	UINT32 data_group;
	UINT32 data_block;
	UINT32 offset = 0;

    parent_inode = parent->entry.inode;
    get_data_block_at_inode(parent->fs, &inode, parent_inode,block_num);		//inode를 읽어오면서 block_num도 배열로 담아서 가져옴

	retEntry->fs = parent->fs;

	for(int i = 0; i < inode.blocks; i++) {
		ZeroMemory(block, sizeof(block));
		data_group = block_num[i] / parent->fs->sb.block_per_group;
		data_block = block_num[i] % parent->fs->sb.block_per_group;
		offset = 0;

		block_read(parent->fs->disk, data_group, data_block, block);
		entry = (EXT2_DIR_ENTRY *)block;

		do {
			if(strcmp((const char *)entry->name, (const char *)entryName) == 0)
			{
				memcpy((unsigned long)&retEntry->entry, (unsigned long)entry, (unsigned long)sizeof(EXT2_DIR_ENTRY));
                retEntry->location.group = data_group;
			    retEntry->location.block = data_block;
				retEntry->location.offset = offset * sizeof(EXT2_DIR_ENTRY);
				return EXT2_SUCCESS;
			}
	      	offset++;
			entry++;

	    } while(entry->name[0] != DIR_ENTRY_NO_MORE && offset != MAX_BLOCK_SIZE/sizeof(EXT2_DIR_ENTRY));
	}	

	return 1;			//directory에 entryName에 해당하는 파일이 없는 경우 
}

int ext2_chmod(DISK_OPERATIONS* disk, EXT2_FILESYSTEM* fs,EXT2_NODE* parent, EXT2_NODE* retentry,int rwxmode)
{
	INODE inode;
	UINT umode;
	UINT gmode;
	UINT othermode;
	
    get_inode(disk,&fs->sb,retentry->entry.inode, &inode);
    
	umode = rwxmode/100;
	if(umode!=0) gmode=(rwxmode-umode*100)/10;
	else gmode=rwxmode/10;
    othermode = rwxmode%10; 

	if( umode>=8 || umode<0 || gmode>=8 || othermode>=8) return EXT2_ERROR;
    inode.mode = (inode.mode&0xFE00)+(umode<<6)+(gmode<<3)+(othermode);
    
	set_inode(disk,&fs->sb,retentry->entry.inode,&inode);
    return EXT2_SUCCESS;
}

/* 'dir'디렉토리에 존재하는 모든 파일을 읽어와 list에 저장하는 함수 */
int ext2_read_dir(EXT2_NODE* dir, EXT2_NODE_ADD adder, void* list)
{
	INODE inode;
	EXT2_NODE file;
	EXT2_DIR_ENTRY* entry;
	UINT32 block_num[MAX_BLOCK_PER_FILE];
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 data_group;
	UINT32 data_block;
	UINT32 offset = 0;

	if(get_data_block_at_inode(dir->fs, &inode, dir->entry.inode, block_num) != EXT2_SUCCESS) 
		return -1;

	file.fs = dir->fs;

	for(int i = 0; i < inode.blocks; i++) {
		ZeroMemory(block, sizeof(block));
		data_group = block_num[i] / dir->fs->sb.block_per_group;
		data_block = block_num[i] % dir->fs->sb.block_per_group;
		offset = 0;

		/* 마지막 그룹에서 data block 개수 다른경우 */
		/* 
		if(data_group == NUMBER_OF_GROUPS) {
			data_group--;
			data_block = dir->fs->sb.block_per_group + data_block - dir->fs->sb.first_data_block_each_group;
		}
		*/

		block_read(dir->fs->disk, data_group, data_block, block);
		entry = (EXT2_DIR_ENTRY *)block;

		do {
			memcpy((unsigned long)&file.entry, (unsigned long)entry, (unsigned long)sizeof(EXT2_DIR_ENTRY));
			file.location.group = data_group;
			file.location.block = data_block;
			file.location.offset = offset*sizeof(EXT2_DIR_ENTRY);
			adder(dir->fs, list, &file);						//ext2_shell.c의 ext2_add_shell_list함수와 연결 
			offset++;
			entry++;
		} while(entry->name[0] != DIR_ENTRY_NO_MORE && (offset*sizeof(EXT2_DIR_ENTRY)) < MAX_BLOCK_SIZE);

	}	

	return EXT2_SUCCESS;
}

int ext2_mkdir(const EXT2_NODE* parent, const char* entryName, EXT2_NODE* retEntry)
{
	INODE inode;
	INODE parent_inode;
	EXT2_DIR_ENTRY entry;
	EXT2_DIR_ENTRY_LOCATION location;
	EXT2_SUPER_BLOCK* sb;
	EXT2_GROUP_DESCRIPTOR* gd;
	BYTE block[MAX_BLOCK_SIZE];
	INT32 b_num;
	INT32 i_num;
	INT32 g_num;
	INT32 g;
	char* dot = ".";
	char* dotdot = "..";

	i_num = alloc_free_inode(parent->fs, parent);

	if(i_num < 0) {
		printf("alloc free inode error\n\r");
		return EXT2_ERROR;
	}

	g_num =  GET_INODE_GROUP(i_num);
	g = g_num;

    do{
		b_num = alloc_free_block(parent->fs, g);		//새로운 block을 할당
           
		if(b_num < 0) return EXT2_ERROR;
		else if(b_num == 0) g = (++g)%NUMBER_OF_GROUPS;	//해당 group에 free block이 존재하지 않는 경우 
		else  {									//해당 group에서 free block을 찾은 경우 
			g_num = g;
		    break;
		}	    
	} while(g != g_num);

	if(b_num == 0) {
		printf("No more free block\n\r");
		return EXT2_ERROR; 
	}

	get_inode(parent->fs->disk, &parent->fs->sb, parent->entry.inode, &parent_inode);	//parent의 inode를 읽어옴

	/* 생성하려는 directory의 inode 초기화, disk에 write */
	inode.mode = 0x01a4 | 0x4000;
	inode.uid = parent_inode.uid;
	inode.gid = parent_inode.gid;
	inode.blocks = 1;								
	inode.block[0] = b_num;					
	inode.size = 2 * sizeof(EXT2_DIR_ENTRY);
	inode.links_count = 2;
	//time(&inode.mtime);

	set_inode(parent->fs->disk, &parent->fs->sb, i_num, &inode);

	/* parent directory의 link개수 증가 */
	parent_inode.links_count += 1;
	set_inode(parent->fs->disk, &parent->fs->sb, parent->entry.inode, &parent_inode);

	ZeroMemory(block, sizeof(block));
	/* dot entry */
	entry.inode = i_num;
	strcpy((char *)entry.name, (char *)dot);
	entry.name_len = strlen(dot);
	
	memcpy((unsigned long)block, (unsigned long)&entry, (unsigned long)sizeof(EXT2_DIR_ENTRY));

	/* dotdot entry */
	entry.inode = parent->entry.inode;
	strcpy((char *)entry.name, (char *)dotdot);
	entry.name_len = strlen(dotdot);

	memcpy((unsigned long)(block + sizeof(EXT2_DIR_ENTRY)), (unsigned long)&entry, (unsigned long)sizeof(EXT2_DIR_ENTRY));
	block_write(parent->fs->disk, g_num, b_num, block);

	/* new directory entry */
	entry.inode = i_num;
	strcpy((char *)entry.name, (char *)entryName);
	entry.name_len = strlen(entryName);

	insert_entry(parent->fs, parent, &entry, &location);

	memcpy((unsigned long)&retEntry->entry, (unsigned long)&entry, (unsigned long)sizeof(EXT2_DIR_ENTRY));
	memcpy((unsigned long)&retEntry->location, (unsigned long)&location, (unsigned long)sizeof(EXT2_DIR_ENTRY_LOCATION));	

	/* superblock 수정 */
	ZeroMemory(block, sizeof(block));
	sb = (EXT2_SUPER_BLOCK *)block;
	block_read(parent->fs->disk, 0, 0, block);
	sb->free_inode_count -= 1;
	block_write(parent->fs->disk, 0, 0, block);
	memcpy((unsigned long)&parent->fs->sb, (unsigned long)sb, (unsigned long)sizeof(EXT2_SUPER_BLOCK));
	memcpy((unsigned long)&retEntry->fs->sb, (unsigned long)sb, (unsigned long)sizeof(EXT2_SUPER_BLOCK));

	/* group descriptor 수정 */
	ZeroMemory(block, sizeof(block));
	gd = (EXT2_GROUP_DESCRIPTOR *)block;
	block_read(parent->fs->disk, 0, 1 + (g_num/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
	gd += (g_num % (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR)));
	gd->free_inodes_count -= 1;
	gd->directories_count += 1;
	block_write(parent->fs->disk, 0, 1 + (g_num/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
	memcpy((unsigned long)&retEntry->fs->gd, (unsigned long)gd, (unsigned long)sizeof(EXT2_GROUP_DESCRIPTOR));

	retEntry->fs = parent->fs;

	return EXT2_SUCCESS;
}

int insert_entry(EXT2_FILESYSTEM* fs, const EXT2_NODE* parent, EXT2_DIR_ENTRY* entry, EXT2_DIR_ENTRY_LOCATION* location)
{
	INODE p_inode;
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 b_num[MAX_BLOCK_PER_FILE];
	int entry_group;
	int entry_block;
	int entry_offset;
	int g;

	get_data_block_at_inode(fs, &p_inode, parent->entry.inode, b_num);

	if((p_inode.mode & 0x4000) == 0) return EXT2_ERROR;	//parent가 directory 파일인지 확인 

	entry_group = b_num[p_inode.blocks - 1] / fs->sb.block_per_group;
	entry_block = b_num[p_inode.blocks - 1] % fs->sb.block_per_group;
	entry_offset = p_inode.size % MAX_BLOCK_SIZE;

	if(entry_offset == 0) { 	//block을 할당해야 하는 경우 
		g = entry_group;
		do {	
			entry_block = alloc_free_block(fs, g);		//새로운 block을 할당
			if(entry_block < 0) return EXT2_ERROR;
			else if(entry_block == 0);			 	//해당 group에 free block이 존재하지 않는 경우 
			else {									//해당 greoup에서 free block을 찾은 경우 
				entry_group = g;
				break;
			}
			g++;
			g = g % NUMBER_OF_GROUPS;
		} while (g != entry_group);

		if(entry_block == 0) {		//free block이 존재하지 않는 경우 
			printf("free block does not exist\n\r");
			return EXT2_ERROR; 	
		}
		
		set_inode_block(fs, parent->entry.inode, entry_group * fs->sb.block_per_group + entry_block);

	}

	else {	
		p_inode.size += sizeof(EXT2_DIR_ENTRY);
		set_inode(fs->disk, &fs->sb, parent->entry.inode, &p_inode);
	}

	location->group = entry_group;
	location->block = entry_block;
	location->offset = entry_offset;

	ZeroMemory(block, sizeof(block));
	block_read(fs->disk, entry_group, entry_block, (unsigned char *)block);
	memcpy((unsigned long)(block+entry_offset), (unsigned long)entry, (unsigned long)sizeof(EXT2_DIR_ENTRY));
	block_write(fs->disk, entry_group, entry_block, block);

	return EXT2_SUCCESS;
}

/* 'i_num'에 해당하는 inode의 파일에서 b_num번째 block의 전체 block번호를 return */
int get_inode_block(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK* sb, UINT32 i_num, UINT32 b_num)
{
	INODE inode;
	UINT32 block[MAX_BLOCK_SIZE/sizeof(UINT32)];
	UINT32 blockPerGroup = sb->block_per_group;
	UINT32 indir_b_num = MAX_BLOCK_SIZE/sizeof(UINT32);
	UINT32 double_b_num = indir_b_num * indir_b_num;
	int g_triple;
	int g_double;
	int g_indirect;
	int b_triple = 0;
	int b_double = 0;
	int b_indirect = 0;
	int temp;

	get_inode(disk, sb, i_num, &inode);

	if(b_num >= 12 + indir_b_num + double_b_num) {
		ZeroMemory(block, sizeof(block));
		g_triple = inode.block[14] / blockPerGroup;
		b_triple = inode.block[14] % blockPerGroup;
		block_read(disk, g_triple, b_triple, (unsigned char *)block);

		temp = (b_num - 12 - indir_b_num - double_b_num) / double_b_num;
		g_double = block[temp] / blockPerGroup;
		b_double = block[temp] % blockPerGroup;
		ZeroMemory(block, sizeof(block));
		block_read(disk, g_double, b_double, (unsigned char *)block);

		temp = ((b_num - 12 - indir_b_num - double_b_num)%double_b_num) / indir_b_num;
		g_indirect = block[temp] / blockPerGroup;
		b_indirect = block[temp] % blockPerGroup;
		ZeroMemory(block, sizeof(block));
		block_read(disk, g_indirect, b_indirect, (unsigned char *)block);

		temp = (b_num - 12 - indir_b_num - double_b_num)%indir_b_num;

		return block[temp];
	}

	else if(b_num >= 12 + indir_b_num) {
		g_double = inode.block[13] / blockPerGroup;
		b_double = inode.block[13] % blockPerGroup;
		ZeroMemory(block, sizeof(block));
		block_read(disk, g_double, b_double, (unsigned char *)block);
		temp = (b_num - 12 - indir_b_num)/indir_b_num;

		g_indirect = block[temp] / blockPerGroup;
		b_indirect = block[temp] % blockPerGroup;

		ZeroMemory(block, sizeof(block));
		block_read(disk, g_indirect, b_indirect, (unsigned char *)block);
		temp = (b_num - 12 - indir_b_num)%indir_b_num;
		return block[temp];
	}

	else if(b_num >= 12) {
		g_indirect = inode.block[12] / blockPerGroup;
		b_indirect = inode.block[12] % blockPerGroup;
		ZeroMemory(block, sizeof(block));
		block_read(disk, g_indirect, b_indirect, (unsigned char *)block);

		temp = b_num - 12;
		return block[temp];
	}

	else {
		return inode.block[b_num];
	}
}

/* 'i_num'에 해당하는 inode에 block의 개수 하나 추가하고 data block 번호를 저장 */
int set_inode_block(EXT2_FILESYSTEM* fs, UINT32 i_num, UINT32 b_num)
{
	INODE inode;
	EXT2_SUPER_BLOCK sb;
	UINT32 i_group = GET_INODE_GROUP(i_num);
	UINT32 blocks;
	UINT32 block[MAX_BLOCK_SIZE/sizeof(UINT32)];
	UINT32 blockPerGroup = fs->sb.block_per_group;
	UINT32 indir_b_num = MAX_BLOCK_SIZE/sizeof(UINT32);
	UINT32 double_b_num = indir_b_num * indir_b_num;
	//UINT32 triple_b_num = double_b_num * indir_b_num;
	int g_triple = i_group;
	int g_double = i_group;
	int g_indirect = i_group;
	int b_triple = 0;
	int b_double = 0;
	int b_indirect = 0;
	int temp;

	get_inode(fs->disk, &fs->sb, i_num, &inode);

	blocks = inode.blocks;

	ZeroMemory(block, sizeof(block));
	block_read(fs->disk, 0, 0, (unsigned char *)block);
	memcpy((unsigned long)&sb, (unsigned long)block, (unsigned long)sizeof(EXT2_SUPER_BLOCK));

	if(blocks < 12) {		//data block의 개수가 12개 미만인 경우 
		inode.block[blocks] = b_num;
		inode.blocks = inode.blocks + 1;
		if((inode.mode & 0x4000) != 0)
			inode.size += sizeof(EXT2_DIR_ENTRY);

		set_inode(fs->disk, &fs->sb, i_num, &inode);

		return EXT2_SUCCESS;
	}

	if(blocks >= 12 + indir_b_num + double_b_num) {				//triple indirect 위치블록을 사용하는 경우 

		if(blocks == 12 + indir_b_num + double_b_num) {			//triple indirect 위치블록을 할당해야 하는 경우 
			g_triple = i_group;
			do {
				b_triple = alloc_free_block(fs, g_triple);

				if(b_triple < 0) return EXT2_ERROR;
				else if(b_triple == 0) {
					g_triple++;
					g_triple = g_triple % NUMBER_OF_GROUPS;
				}
				else break;

			} while(g_triple != i_group);

			if(b_triple == 0) {
				printf("free block does not exist\n\r");
				return EXT2_ERROR; 
			}

			inode.block[14] = g_triple * blockPerGroup + b_triple;			//inode.block[14]에 triple indirect block 번호 저장
		}
		else {							//triple indirect block의 group번호와 block번호를 구함
			g_triple = inode.block[14] / blockPerGroup;
			b_triple = inode.block[14] % blockPerGroup;
		}
		
	}

	if(blocks >= 12 + indir_b_num) {				//double indirect block을 사용하는 경우 

		if((blocks - 12 - indir_b_num)%double_b_num == 0) {				//double indirect 위치블록 할당해야 하는 경우 

			g_double = g_triple;				
			do {
				b_double = alloc_free_block(fs, g_double);

				if(b_double < 0) return EXT2_ERROR;
				else if(b_double == 0) {
					g_double++;
					g_double = g_double % NUMBER_OF_GROUPS;
				}
				else break;

			} while(g_double != g_triple);

			if(b_double == 0) {
				printf("free block does not exist\n\r");
				return EXT2_ERROR; 
			}

			if((blocks - 12 - indir_b_num) == 0) {
				inode.block[13] = g_double * blockPerGroup + b_double;
			}
			else {
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_triple, b_triple, (unsigned char *)block);
				temp = (blocks - 12 - indir_b_num - double_b_num) / double_b_num;
				block[temp] = g_double * blockPerGroup + b_double;
				block_write(fs->disk, g_triple, b_triple, (unsigned char *)block);
			}
		
		}
		else {
			if(blocks < 12 + indir_b_num + double_b_num) {
				g_double = inode.block[13] / blockPerGroup;
				b_double = inode.block[13] % blockPerGroup;
			}
			else {
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_triple, b_triple, (unsigned char *)block);
				temp = (blocks - 12 - indir_b_num - double_b_num) / double_b_num;
				g_double = block[temp] / blockPerGroup;
				b_double = block[temp] % blockPerGroup;
			}
		}
	}
	
	if(blocks >= 12) {			//indirect 위치블록을 사용하는 경우 

		if((blocks - 12)%indir_b_num == 0) {		//indirect 위치블록 할당
			g_indirect = g_double;				
			do {
				b_indirect = alloc_free_block(fs, g_indirect);

				if(b_indirect < 0) {
					return EXT2_ERROR;
				}
				else if(b_indirect == 0) {
					g_indirect++;
					g_indirect = g_indirect % NUMBER_OF_GROUPS;
				}
				else break;
			
			} while(g_indirect != g_double);

			if(b_indirect == 0) {
				printf("free block does not exist\n\r");
				return EXT2_ERROR; 
			}

			if(blocks >= 12 + indir_b_num + double_b_num) {
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_double, b_double, (unsigned char *)block);
				temp = ((blocks - 12 - indir_b_num - double_b_num)%double_b_num) / indir_b_num;
				block[temp] = g_indirect * blockPerGroup + b_indirect; 
				block_write(fs->disk, g_double, b_double, (unsigned char *)block);

			}

			else if(blocks >= 12 + indir_b_num) {
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_double, b_double, (unsigned char *)block);
				temp = (blocks - 12 - indir_b_num)/indir_b_num;
				block[temp] = g_indirect * blockPerGroup + b_indirect;
				block_write(fs->disk, g_double, b_double, (unsigned char *)block);
		
			}

			else {
				inode.block[12] = g_indirect * blockPerGroup + b_indirect;
			}

			ZeroMemory(block, sizeof(block));	
			block[0] = b_num;
			block_write(fs->disk, g_indirect, b_indirect, (unsigned char *)block); 

		}

		else {
			if(blocks > 12 + indir_b_num + double_b_num) {
				ZeroMemory(block, sizeof(block));
		 		block_read(fs->disk, g_double, b_double, (unsigned char *)block);
				temp = ((blocks - 12 - indir_b_num - double_b_num)%double_b_num) / indir_b_num;
				g_indirect = block[temp] / blockPerGroup;
				b_indirect = block[temp] % blockPerGroup;
				temp = (blocks - 12 - indir_b_num - double_b_num)%indir_b_num;
			}
			else if(blocks > 12 + indir_b_num) {
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_double, b_double, (unsigned char *)block);
				temp = (blocks - 12 - indir_b_num) / indir_b_num;
				g_indirect = block[temp] / blockPerGroup;
				b_indirect = block[temp] % blockPerGroup;
				
				temp = (blocks - 12 - indir_b_num)%indir_b_num;
			}

			else {
				g_indirect = inode.block[12] / blockPerGroup;
				b_indirect = inode.block[12] % blockPerGroup;

				temp = blocks - 12;
			}

			ZeroMemory(block, sizeof(block));
			block_read(fs->disk, g_indirect, b_indirect, (unsigned char *)block);
			block[temp] = b_num;
			block_write(fs->disk, g_indirect, b_indirect, (unsigned char *)block);
		}

	}

	inode.blocks = inode.blocks + 1;
	if((inode.mode & 0x4000) != 0)
		inode.size += sizeof(EXT2_DIR_ENTRY);

	set_inode(fs->disk, &fs->sb, i_num, &inode);

	return EXT2_SUCCESS;
}

/* 'group'번째 그룹에서 free data block을 찾고 group 내에서의 block번호를 리턴   */
int alloc_free_block(const EXT2_FILESYSTEM* fs, UINT32 group) 	
{
	EXT2_GROUP_DESCRIPTOR* gd;
	EXT2_SUPER_BLOCK* sb;
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 b_bitmap = fs->gd.start_block_of_block_bitmap;
	UINT32 datablockPerGroup = fs->sb.block_per_group;
	UINT32 max_bit = datablockPerGroup / 8;
	int bit;
	int offset;
	int check = 0;
	UINT32 b_num;

	ZeroMemory(block, sizeof(block));
	block_read(fs->disk, group, b_bitmap, block);

	for(offset = (fs->sb.first_data_block_each_group / 8); offset < max_bit; offset++) {
		if(block[offset] == 0xff) continue;

		BYTE mask = 0x01;
	
		for(bit = 0; bit < 8 ; bit++){
			if(block[offset] & mask) mask = mask << 1; 
			else {
				check = 1;
				break;
			} 
		}	
		if(check == 1) break;
		
	}

	if(offset == max_bit && (datablockPerGroup%8) != 0) {
		BYTE mask = 0x01;

		for(bit = 0; bit < (datablockPerGroup%8); bit++) {
			if(block[offset] & mask) mask = mask << 1;
			else {
				check = 1;
				break;
			}
		}
		
		if(check == 0) return 0;		//해당 group에서 free data block이 없는 경우 
	}
	else if(offset == max_bit) return 0;	//해당 group에서 free data block이 없는 경우 

	b_num = offset * 8 + bit;
	ZeroMemory(block, sizeof(block));
	block_write(fs->disk, group, b_num, block);
	set_block_bitmap(fs->disk, group, b_num, 1);

	/* super block 수정 */
	block_read(fs->disk, 0, 0, block);
	sb = (EXT2_SUPER_BLOCK *)block;
	sb->free_block_count -= 1;
	block_write(fs->disk, 0, 0, block);

	/* group descriptor 수정 */
	ZeroMemory(block, sizeof(block));
	gd = (EXT2_GROUP_DESCRIPTOR *)block;
	block_read(fs->disk, 0, 1 + (group/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
	gd += (group % (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR)));
	gd->free_blocks_count -= 1;
	block_write(fs->disk, 0, 1 + (group/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);

	printf("group : %d block : %d\n\r", group, b_num);

	return b_num;
}

int alloc_free_inode(EXT2_FILESYSTEM* fs, const EXT2_NODE* parent)
{
	EXT2_GROUP_DESCRIPTOR* gd;
	BYTE block[MAX_BLOCK_SIZE];
	BYTE compare = 0;
	UINT32 free_inode_number = 0;
	UINT32 bitmap_block = fs->gd.start_block_of_inode_bitmap;
	UINT32 g_num;	
	int j = 0;
	int check = 0;
	int gd_block = (parent->location.group / (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR)));	//parent의 group descriptor가 gdt의 몇번째 block에 있는지
	int i = gd_block;

	for(int g = 0; g < NUMBER_OF_GROUPS; g++) {
		ZeroMemory(block, sizeof(block));
		block_read(fs->disk, 0, 1 + (g/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
		gd = (EXT2_GROUP_DESCRIPTOR *)block;
		gd += (g % (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR)));
		
		if(gd->free_inodes_count < 1) continue;
		if(gd->free_blocks_count < 3) continue;
		
		check = 1;
		g_num = g;
		break;
	}

	if(check == 0) {
		printf("No free inode\n\r");
		return EXT2_ERROR;
	}

	ZeroMemory(block,sizeof(block));
	block_read(fs->disk, g_num, bitmap_block, block);	
 	i = 0;	
 	while(i<MAX_BLOCK_SIZE){
		compare = block[i];
	    for( j=0;j<8;j++)
		{
			if(compare & 0x01) compare = compare>>1;
			else 
			{
				free_inode_number = g_num*(fs->sb.inode_per_group) + (i*8) + j + 1;
		        set_inode_bitmap(fs->disk, free_inode_number, 1);
			    return free_inode_number;
			}
	    }
	 
		i++;
		if(i*8 + j == fs->sb.inode_per_group)
		{
			return EXT2_ERROR;

		}
		
    }

	return EXT2_ERROR;

}

int release_inode(EXT2_FILESYSTEM* fs, EXT2_NODE* entry)		//entry의 inode와 entry가 사용하는 block을 모두 해제하는 함수 
{
	EXT2_SUPER_BLOCK* sb;
	EXT2_GROUP_DESCRIPTOR* gd;
	INODE inode;
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 block_num[MAX_BLOCK_PER_FILE];
	UINT32 group[NUMBER_OF_GROUPS] = { 0, };
	UINT32 blocks = inode.blocks;

	get_data_block_at_inode(fs, &inode, entry->entry.inode, block_num);		//해제하려는 파일의 inode를 읽어옴
	set_inode_bitmap(fs->disk, entry->entry.inode, 0);		//해당 inode를 bitmap에서 0으로 set

	release_block(fs, &inode, block_num, inode.blocks, group);		//inode의 모든 block을 release함 

	/* super block 수정 */
	ZeroMemory(block, sizeof(block));
	block_read(fs->disk, 0, 0, block);
	sb = (EXT2_SUPER_BLOCK *)block;
	sb->free_block_count += blocks;
	sb->free_inode_count += 1;
	block_write(fs->disk, 0, 0, block);
	memcpy((unsigned long)&fs->sb, (unsigned long)sb, (unsigned long)sizeof(EXT2_SUPER_BLOCK));

	/* group descriptor 수정 */
	for(int g = 0; g < NUMBER_OF_GROUPS; g++) {
		if(group[g] == 0) continue;
		ZeroMemory(block, sizeof(block));
		block_read(fs->disk, 0, 1 + (g/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
		gd = (EXT2_GROUP_DESCRIPTOR *)block;
		gd += (g % (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR)));
		gd->free_blocks_count += group[g];
		if(g == GET_INODE_GROUP(entry->entry.inode)) {
			gd->free_inodes_count += 1;
			gd->directories_count -= 1;
		}
		block_write(fs->disk, 0, 1 + (g/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
		memcpy((unsigned long)&fs->gd, (unsigned long)gd, (unsigned long)sizeof(EXT2_GROUP_DESCRIPTOR));
	}

	return EXT2_SUCCESS;

}

int release_dir_entry(EXT2_FILESYSTEM* fs, EXT2_NODE* parent, EXT2_NODE* entry)
{
	INODE p_inode;
	EXT2_DIR_ENTRY dir;
	EXT2_DIR_ENTRY *dir_p;
	UINT32 block_num[MAX_BLOCK_PER_FILE];
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 group[NUMBER_OF_GROUPS] = { 0, };
	UINT32 g_num;
	UINT32 b_num;
	UINT32 offset;

	/* directory의 마지막 entry를 dir 변수에 읽어옴 */
	ZeroMemory(block, sizeof(block));
	get_data_block_at_inode(fs, &p_inode, parent->entry.inode, block_num);		
	g_num = block_num[p_inode.blocks - 1]/fs->sb.block_per_group;
	b_num = block_num[p_inode.blocks - 1]%fs->sb.block_per_group;
	offset = (p_inode.size - sizeof(EXT2_DIR_ENTRY))%MAX_BLOCK_SIZE;
	block_read(fs->disk, g_num, b_num, block);
	dir_p = (EXT2_DIR_ENTRY *)block;
	dir_p = dir_p + (offset/sizeof(EXT2_DIR_ENTRY));
	memcpy((unsigned long)&dir, (unsigned long)dir_p, (unsigned long)sizeof(EXT2_DIR_ENTRY));
	if((offset/sizeof(EXT2_DIR_ENTRY)) == 0) {			//마지막 entry가 block내에서 첫 entry인 경우 block을 해제 
		release_block(fs, &p_inode, block_num, 1, group);
		increase_free_block(fs, group);
	}
	else {												//마지막 entry가 block내에서 첫 entry가 아닌 경우 마지막 entry위치에 더이상 entry가 없음을 표시 
		dir_p->name[0] = DIR_ENTRY_NO_MORE;
		block_write(fs->disk, g_num, b_num,block);
	}

	/* 마지막 entry를 해제할 entry의 위치로 옮김 */
	ZeroMemory(block, sizeof(block));
	block_read(fs->disk, entry->location.group, entry->location.block, block);
	dir_p = (EXT2_DIR_ENTRY *)block;
	dir_p += entry->location.offset/sizeof(EXT2_DIR_ENTRY);

	if(dir_p->inode == dir.inode) {						//삭제하려는 entry가 마지막 entry인 경우 읽어온 마지막 entry를 DIR_ENTRY_NO_MORE로 변경 후에 덮어씀
		dir.name[0] = DIR_ENTRY_NO_MORE;
		dir.inode = 0;
		dir.name_len = 0;
	}
	memcpy((unsigned long)dir_p, (unsigned long)&dir, (unsigned long)sizeof(EXT2_DIR_ENTRY));
	block_write(fs->disk, entry->location.group, entry->location.block, block);

	/* parent directory의 inode 수정 */
	p_inode.size -= sizeof(EXT2_DIR_ENTRY);
	set_inode(fs->disk, &fs->sb, parent->entry.inode, &p_inode);

	return EXT2_SUCCESS;
}

/* 'inode'구조체에서 사용하는 모든 block의 번호가 b_num배열에 저장되어 있음 해당 
	inode가 사용하는 block에서 마지막 block부터 release_num개의 block을 해제하는 함수 */
int release_block(EXT2_FILESYSTEM* fs, INODE* inode, UINT32* b_num, int release_num, UINT32* group)
{
	UINT32 blocks = inode->blocks;
	UINT32 blockPerGroup = fs->sb.block_per_group;
	UINT32 indir_b_num = MAX_BLOCK_SIZE/sizeof(UINT32);
	UINT32 double_b_num = indir_b_num * indir_b_num;
	UINT32 block[MAX_BLOCK_SIZE/sizeof(UINT32)];
	UINT32 g, b;
	int g_indirect;
	int b_indirect;
	int g_double;
	int b_double;
	int g_triple;
	int b_triple;

	for(int i = 1; i <= release_num; i++) {				//inode가 사용하는 block을 release_num개만큼 해제 
		g = b_num[blocks - i] / blockPerGroup;
		b = b_num[blocks - i] % blockPerGroup;
		set_block_bitmap(fs->disk, g, b, 0);
		group[g] += 1;
	}

	for(int i = 1; i <= release_num; i++) {
		if(blocks - i >= 12 + indir_b_num + double_b_num) {									//triple indirect 위치블록을 사용하는 경우 
			if(((blocks - i) - 12 - indir_b_num - double_b_num) % indir_b_num == 0) {		//indirect block해제하는 경우 
				g_triple = inode->block[14] / blockPerGroup;
				b_triple = inode->block[14] % blockPerGroup;
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_triple, b_triple, (unsigned char *)block);

				g_double = block[(blocks - i - 12 - indir_b_num - double_b_num)/double_b_num] / blockPerGroup;
				b_double = block[(blocks - i - 12 - indir_b_num - double_b_num)/double_b_num] % blockPerGroup;
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_double, b_double, (unsigned char *)block);

				g_indirect = block[((blocks - i - 12 - indir_b_num - double_b_num)%double_b_num)/indir_b_num] / blockPerGroup;
				b_indirect = block[((blocks - i - 12 - indir_b_num - double_b_num)%double_b_num)/indir_b_num] % blockPerGroup;
				set_block_bitmap(fs->disk, g_indirect, b_indirect, 0);
				group[g_indirect] += 1;

				if(((blocks - i) - 12 - indir_b_num - double_b_num) % double_b_num == 0) {		//double indirect block해제하는 경우 
					set_block_bitmap(fs->disk, g_double, b_double, 0);
					group[g_double] += 1;

					if((blocks - i) - 12 - indir_b_num - double_b_num == 0) {					//triple indirect block해제하는 경우 
						set_block_bitmap(fs->disk, g_triple, b_triple, 0);
						group[g_triple] += 1;
						inode->block[14] = 0;
					}
				}
 			}

		}

		else if(blocks - i >= 12 + indir_b_num) {									//double indirect 위치블록을 사용하는 경우 
			if(((blocks - i) - 12 - indir_b_num) % indir_b_num == 0) {				//indirect block해제하는 경우 
				g_double = inode->block[13] / blockPerGroup;
				b_double = inode->block[13] % blockPerGroup;
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_double, b_double, (unsigned char *)block);
				g_indirect = block[((blocks - i) - 12 - indir_b_num) / indir_b_num] / blockPerGroup;
				b_indirect = block[((blocks - i) - 12 - indir_b_num) / indir_b_num] % blockPerGroup;

				set_block_bitmap(fs->disk, g_indirect, b_indirect, 0);
				group[g_indirect] += 1;
	
				if(blocks - i - 12 - indir_b_num == 0) {							//double indirect block 해제하는 경우 
					set_block_bitmap(fs->disk, g_double, b_double, 0);
					group[g_double] += 1;
					inode->block[13] = 0;
				}
			}
			
		}

		else if(blocks - i == 12) {									//indirect 위치블록을 사용하는 경우 
			g_indirect = inode->block[12] / blockPerGroup;
			b_indirect = inode->block[12] % blockPerGroup;
			set_block_bitmap(fs->disk, g_indirect, b_indirect, 0);
			group[g_indirect] += 1;
			inode->block[12] = 0;
		} 

		else {
			inode->block[blocks - i] = 0;
		}

	}

	inode->blocks -= release_num;

	return EXT2_SUCCESS;
	 
}

/* group배열의 각 인덱스에 해당하는 group에서 배열의 값만큼 free block개수를 증가시킴  */
int increase_free_block(EXT2_FILESYSTEM* fs, UINT32* group)
{
	EXT2_SUPER_BLOCK* sb;
	EXT2_GROUP_DESCRIPTOR* gd;
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 blocks = 0;

	for(int i = 0; i < NUMBER_OF_GROUPS; i++) {			//수정 할 총 블록의 개수를 구함 
		blocks += group[i];
	}

	/* super block 수정 */
	ZeroMemory(block, sizeof(block));
	block_read(fs->disk, 0, 0, block);
	sb = (EXT2_SUPER_BLOCK *)block;
	sb->free_block_count += blocks;
	
	block_write(fs->disk, 0, 0, block);
	memcpy((unsigned long)&fs->sb, (unsigned long)sb, (unsigned long)sizeof(EXT2_SUPER_BLOCK));

	/* group descriptor 수정 */
	for(int g = 0; g < NUMBER_OF_GROUPS; g++) {
		if(group[g] == 0) continue;
		ZeroMemory(block, sizeof(block));
		block_read(fs->disk, 0, 1 + (g/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
		gd = (EXT2_GROUP_DESCRIPTOR *)block;
		gd += (g % (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR)));
		gd->free_blocks_count += group[g];

		block_write(fs->disk, 0, 1 + (g/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
		memcpy((unsigned long)&fs->gd, (unsigned long)gd, (unsigned long)sizeof(EXT2_GROUP_DESCRIPTOR));
	}

	return EXT2_SUCCESS;

}