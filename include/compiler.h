#ifndef _COMPILER_H_
#define _COMPILER_H_

#define barrier() __asm__ __volatile__("": : :"memory")

typedef __signed__ char __s8;
typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

typedef __signed__ long long __s64;
typedef unsigned long long __u64;

static inline void __write_once_size(volatile void *p, void *res, int size)
{
    switch (size) {
	case 1: *(volatile __u8 *)p = *(__u8 *)res; break;
	case 2: *(volatile __u16 *)p = *(__u16 *)res; break;
	case 4: *(volatile __u32 *)p = *(__u32 *)res; break;
	case 8: *(volatile __u64 *)p = *(__u64 *)res; break;
	default:
		barrier();
		__builtin_memcpy((void *)p, (const void *)res, size);
		barrier();
	}
}

/* x에 val를 write */
#define WRITE_ONCE(x, val) \
({                          \
    union { typeof(x) __val; char __c[1]; } __u = \
        { .__val = (typeof(x)) (val) }; \
    __write_once_size(&(x), __u.__c, sizeof(x)); \
    __u.__val;      \
})

#endif