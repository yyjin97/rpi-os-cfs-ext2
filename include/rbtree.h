#ifndef _RBTREE_H
#define _RBTREE_H 

#include "types.h"

#ifndef __ASSEMBLER__

#ifndef NULL
#define NULL ((void *)0)
#endif

struct rb_node {
    unsigned long __rb_parent_color;
    struct rb_node *rb_right;
    struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));

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

#define RB_RED      0
#define RB_BLACK    1

#define __rb_color(pc)      ((pc) & 1)
#define __rb_is_black(pc)   __rb_color(pc)
#define __rb_is_red(pc)     (!__rb_color(pc))
#define rb_color(rb)        __rb_color((rb)->__rb_parent_color)
#define rb_is_red(rb)       __rb_is_red((rb)->__rb_parent_color)
#define rb_is_black(rb)     __rb_is_black((rb)->__rb_parent_color)

#define RB_ROOT (struct rb_root) { NULL, }
#define RB_ROOT_CACHED (struct rb_root_cached) { { NULL, }, NULL }
#define rb_entry(ptr, type, member) container_of(ptr,type,member)

/* empty node는 아직 rbtree에 삽입되지 않은 노드를 나타냄 */
#define RB_EMPTY_NODE(node) ((node)->__rb_parent_color == (unsigned long)(node))

#define rb_first_cached(root) (root)->rb_leftmost

/* parent node를 return */
#define rb_parent(r) ((struct rb_node *)((r)->__rb_parent_color & ~3))
#define __rb_parent(pc) ((struct rb_node *)(pc & ~3))

#endif
#endif