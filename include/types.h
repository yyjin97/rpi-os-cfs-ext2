#ifndef _TYPES_H
#define _TYPES_H

#ifndef __ASSEMBLER__

#ifndef NULL
#define NULL                ((void *)0)
#endif

typedef signed char         s8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;

typedef signed char         s8;
typedef short               s16; 
typedef int                 s32;
typedef long long           s64; 

typedef int		            pid_t;

typedef int                 bool;
#define false 0
#define true 1

#define BYTE				unsigned char
#define WORD				unsigned short
#define DWORD				unsigned int
#define QWORD				unsigned long long int
#define SHORT				short
#define USHORT				unsigned short
#define INT					int
#define UINT				unsigned int
#define INT8				char
#define UINT8				unsigned char
#define INT16				SHORT
#define UINT16				USHORT
#define INT32				INT
#define UINT32				UINT
#define INT64				long long int
#define UINT64				unsigned INT64

#endif /* __ASSEMBLER__ */
#endif /* _TYPES_H */
