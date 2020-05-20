
#include "shell.h"
#include "disksim.h"
#include "printf.h"
#include "mini_uart.h"
#include "string.h"
#include "mm.h"

#define		SECTOR					DWORD

#define COND_MOUNT				0x01
#define COND_UMOUNT				0x02

typedef struct
{
	char*	name;
	int(*handler)(int, char**);
	char	conditions;
} COMMAND;

extern void shell_register_filesystem(SHELL_FILESYSTEM*);
extern void printf_by_sel(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, const char* name, int sel, int num);

void do_shell(void);
void unknown_command(void);
int seperate_string(char* buf, char* ptrs[]);

int shell_cmd_format(int argc, char* argv[]);
int shell_cmd_mount(int argc, char* argv[]);
int shell_cmd_touch(int argc, char* argv[]);
int shell_cmd_cd(int argc, char* argv[]);
int shell_cmd_ls(int argc, char* argv[]);
int shell_cmd_df(int argc, char* argv[]);
int shell_cmd_mv(int argc, char* argv[]);
int shell_cmd_rm(int argc, char* argv[]);
int shell_cmd_cat(int argc, char* argv[]);
int shell_cmd_mkdir(int argc, char* argv[]);
int shell_cmd_mkdirst(int argc, char* argv[]);
int shell_cmd_umount(int argc, char* argv[]);
int shell_cmd_rmdir(int argc, char* argv[]);
int shell_cmd_dumpsuperblock(int argc, char * argv[]);
int shell_cmd_fill(int argc, char* argv[]);
int shell_cmd_dumpdatablockbynum(int argc, char * argv[]);
int shell_cmd_exit(int argc, char *argv[]);
int shell_cmd_chmod(int argc, char * argv[]);
int shell_cmd_pwd(int argc, char* argv[]);

static COMMAND g_commands[] =              //{name,handler,condition}
{
	{ "cd",		shell_cmd_cd,		COND_MOUNT	},
	{ "mount",	shell_cmd_mount,	COND_UMOUNT	},
	{ "touch",	shell_cmd_touch,	COND_MOUNT	},
	{ "fill",	shell_cmd_fill,		COND_MOUNT	},
	{ "ls",		shell_cmd_ls,		COND_MOUNT	},
	{ "df",		shell_cmd_df, 		COND_MOUNT	},
	{ "mv", 	shell_cmd_mv,		COND_MOUNT	},
	{ "rm",		shell_cmd_rm,		COND_MOUNT	},
	{ "cat", 	shell_cmd_cat,		COND_MOUNT 	},
	{ "format",	shell_cmd_format,	COND_UMOUNT	},
	{ "mkdir",	shell_cmd_mkdir,	COND_MOUNT	},
	{ "mkdirst",shell_cmd_mkdirst,	COND_MOUNT 	},
	{ "rmdir", 	shell_cmd_rmdir, 	COND_MOUNT  },
	{ "chmod", 	shell_cmd_chmod, 	COND_MOUNT	},
	{ "pwd", 	shell_cmd_pwd,		COND_MOUNT	},
	{ "umount", shell_cmd_umount,	COND_MOUNT 	},
	{ "exit", 	shell_cmd_exit,		0X00 },
	{ "quit", 	shell_cmd_exit,		0X00 },
	{ "dumpsuperblock" , shell_cmd_dumpsuperblock, COND_MOUNT  },
};

static SHELL_FILESYSTEM		g_fs;
static SHELL_FS_OPERATIONS	g_fsOprs;
static SHELL_ENTRY			g_rootDir;
static SHELL_ENTRY			g_currentDir;
static DISK_OPERATIONS		g_disk;

int g_commandsCount = sizeof(g_commands) / sizeof(COMMAND);
int g_isMounted;

int shell_process(void)
{
	if (disksim_init(&g_disk) < 0)    //disk 초기화 
	{
		printf("disk simulator initialization has been failed\n\r");
		return -1;
	}

	shell_register_filesystem(&g_fs);  

	do_shell();

	return 0;
}

int check_conditions(int conditions)    //mount되어야하는 명령어인데 mount되지 않은경우, mount되지 않아야하는 명령어인데 mount된 경우 error
{
	if (conditions & COND_MOUNT && !g_isMounted)  
	{
		printf("file system is not mounted\n\r");
		return -1;
	}

	if (conditions & COND_UMOUNT && g_isMounted)
	{
		printf("file system is already mounted\n\r");
		return -1;
	}

	return 0;
}

void do_shell(void)
{
	char buf[1000];
	char* argv[100];
	int argc;
	int i;

	printf("%s File system shell\n\r", g_fs.name);

	ZeroMemory(buf, sizeof(buf));

	while (-1)
	{
		printf("[/%s]# ", g_currentDir.name);

		//fgets(buf, 1000, stdin);
		gets(buf, 1000);
		argc = seperate_string(buf, argv);  //사용자 입력 문자열을 공백문자 기준으로 나누어서 argv에 저장, argc에 문자열 개수 저장

		if (argc == 0) //입력이 없는 경우 
			continue;

		for (i = 0; i < g_commandsCount; i++) //command개수만큼 for문을 돌며 확인
		{
			if (strcmp(g_commands[i].name, argv[0]) == 0) //command 이름과 사용자 입력 명령어 문자열 비교 
			{
				if (check_conditions(g_commands[i].conditions) == 0)  //명령어의 mount필요성과 mount여부 판별
					if(g_commands[i].handler(argc, argv) == -2) return;;    //명령어 handler호출 

				break;
			}
		}

		if (argc != 0 && i == g_commandsCount)  //입력은 있는데 일치하는 명령어가 없는 경우 
			unknown_command();
	}
}

void unknown_command(void)
{
	int i;

	printf(" * ");
	for (i = 0; i < g_commandsCount; i++)
	{
		if (i < g_commandsCount - 1)
			printf("%s, ", g_commands[i].name);
		else
			printf("%s", g_commands[i].name);
	}
	printf("\n\r");
}

int seperate_string(char* buf, char* ptrs[])
{
	char prev = 0;
	int count = 0;

	while (*buf)
	{
		if (isspace(*buf))       //buf의 문자가 공백문자인지 판별
			*buf = 0;
		else if (prev == 0)     //이전 문자가 공백문자인 경우 문자열 시작주소 저장
			ptrs[count++] = buf;

		prev = *buf++;
	}

	return count;
}

int shell_cmd_umount(int argc, char* argv[])
{
	g_isMounted = 0;
	g_fs.umount(&g_fsOprs);
	return 0;
}

int shell_cmd_exit(int argc, char * argv[])
{
	if(g_isMounted) shell_cmd_umount(argc, argv);

	disksim_uninit(&g_disk);

	exit_process();

}

int shell_cmd_format(int argc, char * argv[]) 
{
	int		result;
	char*	param = NULL;

	if (argc >= 2)        //명령어 인자가 존재하는 경우 
		param = argv[1];    //첫 번째 인자 저장

	result = g_fs.format(&g_disk, param); 

	if (result < 0)
	{
		printf("%s formatting is failed\n\r", g_fs.name);
		return -1;
	}

	printf("disk has been formatted successfully\n\r");
	return 0;
}

int shell_cmd_dumpsuperblock(int argc, char * argv[])
{
	SHELL_ENTRY entry;
	printf_by_sel(&g_disk, &g_fsOprs, &g_currentDir, &entry, argv[1], 1, 0);
	return 0;
}

static int top = 0;
static SHELL_ENTRY path[512];

int shell_cmd_cd(int argc, char* argv[])
{
	SHELL_ENTRY entry;

	path[0]= g_rootDir;
     
	 if(argc > 2) {
		printf("usage : %s [directory]\n\r", argv[0]);
		return 0;
	 }

	else if (argc == 1)
        top = 0;

	else
	{
		if(strcmp(argv[1], ".") == 0) 
			return 0;
		else if(strcmp(argv[1], "..") == 0 && top > 0)
			top--;
		else {
		  
		    if( g_fsOprs.lookup(&g_disk, &g_currentDir, &entry, argv[1]) ) {
			    printf("directory not found\n\r");
			    return -1;
			}
		    
			else {
				if(entry.isDirectory)
				       path[++top] = entry;
				else {
				    printf("This is not a directory\n\r");
					return -1;
				}
			}
		}
    }

	g_currentDir = path[top];
	return 0;
	
}

int shell_cmd_mount(int argc, char* argv[])
{
	int result;

	if (g_fs.mount == NULL)
	{
		printf("The mount functions is NULL\n\r");
		return 0;
	}

	result = g_fs.mount(&g_disk, &g_fsOprs, &g_rootDir);
	g_currentDir = g_rootDir;
	g_currentDir.parent = &g_rootDir;

	if (result < 0)
	{
		printf("%s file system mounting has been failed\n\r", g_fs.name);
		return -1;
	}
	else
	{
		printf(" : %s file system has been mounted successfully\n\r", g_fs.name);
		g_isMounted = 1;
	}

	return 0;
}

int shell_cmd_touch(int argc, char* argv[])
{
	
	SHELL_ENTRY	entry;
	int			result;

	if (argc < 2)
	{
		printf("usage : touch [files...]\n\r");
		return 0;
	}

	result = g_fsOprs.fileOprs->create(&g_disk, &g_fsOprs, &g_currentDir, argv[1], &entry);

	if (result)
		return -1;

	return 0;
}

int shell_cmd_fill(int argc, char* argv[])
{
	SHELL_ENTRY	entry;
	int			result;
	int 		size;
	char*		buffer;
	char*		tmp;
	
    if(argc!=3)
	{
		printf("usage : fill [file] [size]\n\r");
		return 0;
	}
     
	//sscanf(argv[2],"%d",&size);
	size = atoi(argv[2]);

	if(size < 0) {
		printf("wrong file size\n\r");
		return -1;
	}

	result = g_fsOprs.fileOprs->create(&g_disk, &g_fsOprs, &g_currentDir, argv[1], &entry);

	if(result)
			return -1;

	//buffer = ( char* )malloc( size );
	buffer = (char *)allocate_kernel_page();
	
	ZeroMemory(buffer, size);

	tmp = buffer;
	while( tmp < buffer + size )
	{
		memcpy( (unsigned long)tmp, (unsigned long)"ISeeYou.", (unsigned long)8 );
		tmp += 8;
	}
 
    g_fsOprs.fileOprs->write(&g_disk,&g_fsOprs,&g_currentDir,&entry,0,size, buffer);
	//free(buffer);
	free_page((unsigned long)buffer);
	return 0;
}


int shell_cmd_rm(int argc, char* argv[])
{
	int result;

	if(argc != 2) {
		printf(" usage : %s [file name]\n\r", argv[0]);
		return 0;
	}

	result = g_fsOprs.fileOprs->remove(&g_disk, &g_fsOprs, &g_currentDir, argv[1]);

	if(result != 0) return -1;
	else return 0;
}

/*double get_percentage(unsigned int number, unsigned int total)
{
	return ((double)number) / total * 100.;
}*/

int shell_cmd_df(int argc, char* argv[])
{
	if(argc != 1) {
		printf(" usage : %s \n\r", argv[0]);
		return 0;
	} 

	if(g_fsOprs.df(&g_disk, &g_fs) < 0) 
		return -1;

	return 0;
}

int shell_cmd_mkdir(int argc, char* argv[])
{
	SHELL_ENTRY entry;
	int result;

	if(argc != 2) {
		printf(" usage : %s [directory name] \n\r", argv[0]);
		return 0;
	}

	if(strlen(argv[1]) > 10) {
		printf("%s file name is too long (max size : 10)\n\r", argv[1]);
		return -1;
	}

	for(int i = 0; i < strlen(argv[1]);i++){
		if(argv[1][i] >= 'A' && argv[1][i] <= 'Z');
		else if (argv[1][i] >= 'a' && argv[1][i] <= 'z');
		else {
			printf("Invalid directory name\n\r");
			return -1;
		}
	}

	result = g_fsOprs.mkdir(&g_disk, &g_fsOprs, &g_currentDir, argv[1], &entry);

	if(result < 0) return -1;
	
	return 0;
	
}

int shell_cmd_rmdir(int argc, char* argv[])
{
	if(argc != 2) {
		printf(" usage : %s [directory name]\n\r", argv[0]);
		return 0;
	}

	if(strcmp((const char *)argv[1], ".") == 0) {
		printf("dot file cannot remove\n\r");
		return -1;
	}

	if(strcmp((const char *)argv[1], "..") == 0) {
		printf("dotdot file cannot remove\n\r");
		return -1;
	}

	if(g_fsOprs.rmdir(&g_disk, &g_fsOprs, &g_currentDir, argv[1]) < 0) return -1;

	return 0;
}

int shell_cmd_mkdirst(int argc, char* argv[])
{
	SHELL_ENTRY entry;
	int result;

	if(argc < 2) {
		printf(" usage : %s [directory name1] [directory name2] ...\n\r", argv[0]);
		return 0;
	}

	for(int i = 1; i < argc; i++) {
		for(int j = 0; j < strlen(argv[i]); j++){
			if(argv[i][j] >= 'A' && argv[i][j] <= 'Z');
			else if (argv[i][j] >= 'a' && argv[i][j] <= 'z');
			else {
				printf("Invalid directory name\n\r");
				return -1;
			}
		}
	}

	for(int i = 1; i < argc; i++) {

		result = g_fsOprs.mkdir(&g_disk, &g_fsOprs, &g_currentDir, argv[i], &entry);

		if(result < 0) 
			return -1;
	}

	return 0;
}

int shell_cmd_cat(int argc, char* argv[])
{

	if(argc < 2) {
		printf(" usage : %s [file name1] [file name2] ...\n\r", argv[0]);
		return 0;
	}

	for(int i = 1; i < argc; i++) {
		if(g_fsOprs.cat(&g_disk, &g_fsOprs, &g_currentDir, argv[i]) < 0) 
			return -1;
	}

	return 0;
}

int shell_cmd_ls(int argc, char* argv[])
{
	SHELL_ENTRY_LIST list;
	unsigned int opt = 0;

	if(argc > 2) {
		printf(" usage : %s (flag) \n\r", argv[0]);
		return 0;
	} 
	else if(argc == 2) {
		if(*argv[1] != '-') { 
			printf(" usage : %s (flag)\n\r", argv[0]);
			return 0;
		}

		for(int i = 1; i < strlen(argv[1]); i++) {
			switch (*(argv[1] + i)) {
				case 'a':
					opt |= 0x0001;
					break;

				case 'l':
					opt |= 0x0002;
					break;

				case 'S':
					opt |= 0x0004;
					break;

				case 'r':
					opt |= 0x0008;
					break;

				default:
					printf("%s flag : a(all), l(long), S(size), r(reverse)\n\r", argv[0]);
					return 0;
			}
		}
	}

	if((opt & 0x0004) && (opt & 0x0008)) {
		printf("size flag and reverse flag cannot be set at the same time\n\r");
		return -1;
	}

	init_entry_list(&list);

	if(g_fsOprs.ls(&g_disk, &g_currentDir, &list, opt) < 0) {
		return -1;
	}

	release_entry_list(&list);

	return 0;
}

int shell_cmd_mv(int argc, char* argv[])
{
	if(argc != 3) {
		printf(" usage : %s [file name] [directory name]\n\r", argv[0]);
		return 0;
	}

	g_fsOprs.mv(&g_disk, &g_fsOprs, &g_currentDir, argv[1], argv[2]);

	return EXT2_SUCCESS;

}

int shell_cmd_pwd(int argc, char* argv[])
{
	if(argc!=1) {
		printf(" usage : %s\n\r",argv[0]);
		return 0;
	}

	if(top==0) {
		printf("/\n\r");
		return 0;
	}

	for(int i =0;i<=top; i++) {
		if(i == 0);
		else printf("/%s", path[i].name);
	}
	printf("\n\r");
	return 0;
}

int shell_cmd_chmod(int argc, char * argv[])
{
	int mode=0;
	if(argc!=3)
	{
		printf(" usage: %s  [number]  [file]\n\r",argv[0]);
		return 0;
	}
	
	//sscanf(argv[1],"%d",&mode);
	mode = atoi(argv[1]);

	if(g_fsOprs.chmod(&g_disk,&g_fsOprs, &g_currentDir,argv[2],mode)<0) return -1;
	return 0;
}
