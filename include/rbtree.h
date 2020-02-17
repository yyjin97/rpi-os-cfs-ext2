#ifndef _RBTREE_H
#define _RBTREE_H 

#include "types.h"

#ifndef __ASSEMBLER__

#ifndef NULL
#define NULL ((void *)0)
#endif

struct rb_node {
    unsigned long __rb_parent_color;
    struct rb_node *rb_parent;
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

extern void rb_link_node(struct rb_node *node, struct rb_node *parent, struct rb_node **rb_link);
extern void rb_insert_color_cached(struct rb_node *node, struct rb_root_cached *root, bool leftmost);
extern void rb_erase_cached(struct rb_node *node, struct rb_root_cached *root);

#define RB_ROOT (struct rb_root) { NULL, }
#define RB_ROOT_CACHED (struct rb_root_cached) { { NULL, }, NULL }
#define rb_entry(ptr, type, member) container_of(ptr,type,member)

#define rb_first_cached(root) (root)->rb_leftmost
#define rb_parent(node) (node)->rb_parent

#endif
#endif