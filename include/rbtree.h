#ifndef _LINUX_RBTREE_H
#define _LINUX_RBTREE_H 

#include "kernel.h"

struct rb_node {
    unsigned long __rb_parent_color;
    struct rb_node *rb_right;
    struct rb_node *rb_left;
};

struct rb_root {
    struct rb_node *rb_node;
};

struct rb_root_cached {
    struct rb_root rb_root;
    struct rb_node *rb_leftmost;
};

#define rb_entry(ptr, type, member) container_of(ptr,type,member)

#define rb_first_cached(root) (root)->rb_leftmost

#endif