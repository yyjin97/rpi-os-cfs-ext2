//#include <stdlib.h>
//#include <memory.h>
#include "ext2.h"
#include "disk.h"
#include "disksim.h"
#include "mm.h"

typedef struct
{
	char*	address;
} DISK_MEMORY;

static unsigned short alloc_disk_memory[sizeof(DISK_MEMORY)] = {0,};
static unsigned short alloc_disk[MAX_SECTOR_SIZE*NUMBER_OF_SECTORS] = {0,};

int disksim_read(DISK_OPERATIONS* this, SECTOR sector, void* data);
int disksim_write(DISK_OPERATIONS* this, SECTOR sector, const void* data);

int disksim_init(DISK_OPERATIONS* disk)
{
	if (disk == NULL)
		return -1;

	//disk->pdata = malloc(sizeof(DISK_MEMORY));
	disk->pdata = alloc_disk_memory;
	if (disk->pdata == NULL)
	{
		disksim_uninit(disk);
		return -1;
	}

	//((DISK_MEMORY*)disk->pdata)->address = (char*)malloc(bytesPerSector * numberOfSectors);  //sector 크기만큼 공간할당
	((DISK_MEMORY*)disk->pdata)->address = (char*)alloc_disk;
	if (disk->pdata == NULL)
	{
		disksim_uninit(disk);
		return -1;
	}

	memset(((DISK_MEMORY*)disk->pdata)->address, 0, MAX_SECTOR_SIZE * NUMBER_OF_SECTORS);  //sector영역을 모두 0으로 set

	/* disk 초기화 */
	disk->read_sector = disksim_read;        
	disk->write_sector = disksim_write;        
	disk->numberOfSectors = NUMBER_OF_SECTORS;    
	disk->bytesPerSector = MAX_SECTOR_SIZE;       
	return 0;
}

void disksim_uninit(DISK_OPERATIONS* this)
{
	if (this)
	{
		if (this->pdata);
			//free(this->pdata);  //sector 영역 해제 
	}
}

int disksim_read(DISK_OPERATIONS* this, SECTOR sector, void* data)	//'sector'에 해당하는 번호(disk전체 기준)의 block 내용을 'data'변수에 복사
{
	char* disk = ((DISK_MEMORY*)this->pdata)->address;

	if (sector < 0 || sector >= this->numberOfSectors)
		return -1;

	memcpy(data, &disk[sector * this->bytesPerSector], this->bytesPerSector);

	return 0;
}

int disksim_write(DISK_OPERATIONS* this, SECTOR sector, const void* data)
{
	char* disk = ((DISK_MEMORY*)this->pdata)->address;		//disk memory 시작주소 저장

	if (sector < 0 || sector >= this->numberOfSectors)		//sector번호 유효성 검사
		return -1;

	memcpy(&disk[sector * this->bytesPerSector], data, this->bytesPerSector); 	//해당 sector번호에 해당하는 sector에 data내용 복사 

	return 0;
}

