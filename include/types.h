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

#endif /* __ASSEMBLER__ */
#endif /* _TYPES_H */