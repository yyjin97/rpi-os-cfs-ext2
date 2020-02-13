#ifndef _RBTREE_H
#define _RBTREE_H 

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

/* node의 멤버를 초기화하고 rb_link의 위치에 node를 추가 */
static inline void rb_link_node(struct rb_node *node, struct rb_node *parent, struct rb_node **rb_link)
{
    node->__rb_parent_color = (unsigned long)parent;
    node->rb_left = node->rb_right = NULL;

    *rb_link = node;
}

#define RB_ROOT (struct rb_root) { NULL, }
#define RB_ROOT_CACHED (struct rb_root_cached) { { NULL, }, NULL }
#define rb_entry(ptr, type, member) container_of(ptr,type,member)

#define rb_first_cached(root) (root)->rb_leftmost

#endif