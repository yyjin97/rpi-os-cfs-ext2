#include "rbtree.h"
#include "compiler.h"
#include "printf.h"

/* rb 노드의 색을 color로 set함 (__rb_parent_color 변수의 마지막 bit는 해당 노드의 색을 나타냄) */
static inline void rb_set_parent_color(struct rb_node *rb, struct rb_node *p, int color)
{
    rb->__rb_parent_color = (unsigned long) p | color;
}

/* node가 red인 경우 parent node를 리턴 */
static inline struct rb_node *rb_red_parent(struct rb_node *red)
{
    return (struct rb_node *)red->__rb_parent_color;
}

/* parent의 child를 old에서 new로 변경 */
static inline void __rb_change_child(struct rb_node *old, struct rb_node *new, struct rb_node *parent, struct rb_root *root)
{
    if(parent) {
        if(parent->rb_left == old)
            WRITE_ONCE(parent->rb_left, new);
        else 
            WRITE_ONCE(parent->rb_right, new);
    } else 
        WRITE_ONCE(root->rb_node, new);
}

/* old의 parent를 new의 parent로 변경, new를 old의 parent로 변경하고 color를 old의 color로 set,
    old의 parent의 child를 new로 변경 */
static inline void __rb_rotate_set_parents(struct rb_node *old, struct rb_node *new, struct rb_root *root, int color)
{
    struct rb_node *parent = rb_parent(old);
    new->__rb_parent_color = old->__rb_parent_color;
    rb_set_parent_color(old, new, color);
    __rb_change_child(old, new, parent, root);
}

/* node의 멤버를 초기화하고 rb_link의 위치에 node를 추가 */
void rb_link_node(struct rb_node *node, struct rb_node *parent, struct rb_node **rb_link)
{
    node->__rb_parent_color = (unsigned long)parent;
    node->rb_left = node->rb_right = NULL;

    *rb_link = node;
}

struct rb_node *rb_next(const struct rb_node *node)
{
    struct rb_node *parent;

    if(RB_EMPTY_NODE(node))
        return NULL;

    /* right-hand child가 존재하면 right child의 가장 left node를 찾음 */
    if(node->rb_right) {
        node = node->rb_right;
        while(node->rb_left)
            node = node->rb_left;
        return (struct rb_node *)node;
    }

    /* right-hand child가 존재하지 않으면 현재 node의 아래쪽 node들은 모두 현재 노드보다 작음
        따라서 next node를 찾으려면 parent 노드쪽으로 올라가야함 
        그러나 현재 노드가 parent node의 right-hand child이면 parent node는 현재 노드보다 작다는 뜻임 
        따라서 현재 노드가 parent node의 left-hand child가 되는 첫 번째 node가 현재노드 다음으로 큰 next node가 됨 */
    while((parent = rb_parent(node)) && node == parent->rb_right)
        node = parent;

    return parent;
}

void __rb_insert(struct rb_node *node, struct rb_root_cached *root)
{
    struct rb_node *parent = rb_red_parent(node);
    struct rb_node *gparent, *tmp;

    while(true) {

        if(!parent) {
            /* parent가 존재하지 않는 경우 root node임 */
            rb_set_parent_color(node, NULL, RB_BLACK);
            break;
        }

        /* parent가 black인 경우 종료 */
        if(rb_is_black(parent))
            break;

        gparent = rb_red_parent(parent); 

        tmp = gparent->rb_right;
        if(parent != tmp) {             //parent node가 gparent의 rb_left인 경우 
            if(tmp && rb_is_red(tmp)) {
                /*
				 * Case 1 - uncle node가 red인 경우 (recoloring을 통해 double red를 해결)
                 * (아래 그래프에서는 소문자가 red, 대문자가 black을 나타냄)
				 *
				 *       G            g
				 *      / \          / \
				 *     p   u  -->   P   U
				 *    /            /
				 *   n            n
				 *
				 * Recoloring은 현재 노드의 부모(parent)와 그 형제(uncle node-tmp)를 black으로 하고 
                 * 부모의 부모(gparent)를 red로 함 
                 * 그러나 만약 gparent의 부모노드가 red인 경우 또다시 double red가 발생하므로 
                 * gparent 노드를 기준으로 다시 반복해야함 
				 */
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
            }

            /* uncle node가 black인 경우에는 restructuring을 해야함 */
            tmp = parent->rb_right;
            if(node == tmp) {
                /*
				 * Case 2 - uncle node가 black이고 node가 parent의 right child인 경우 
				 *
				 *      G             G
				 *     / \           / \
				 *    p   U  -->    n   U
				 *     \           /
				 *      n         p
				 *
				 * 아직 red node의 두 child는 모두 black이라는 규칙을 위반할 수 있지만
                 * Case 3을 통해 해결하게 될 것임
				 */
                tmp = node->rb_left;
                WRITE_ONCE(parent->rb_right, tmp);
                WRITE_ONCE(node->rb_left, parent);
                if(tmp)
                    rb_set_parent_color(tmp, parent, RB_BLACK);
                rb_set_parent_color(parent, node, RB_RED);
                parent = node;
                tmp = node->rb_right;
            }

            /*
			 * Case 3 - uncle node가 black이고 node가 parent의 left child인 경우 
			 *
			 *        G           P
			 *       / \         / \
			 *      p   U  -->  n   g
			 *     /                 \
			 *    n                   U
			 */
            WRITE_ONCE(gparent->rb_left, tmp);
            WRITE_ONCE(parent->rb_right, gparent);
            if(tmp)
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            __rb_rotate_set_parents(gparent, parent, &root->rb_root, RB_RED);
            break;
        } else {            //parent 노드가 gparent 노드의 right child인 경우 
            tmp = gparent->rb_left;

            if(tmp && rb_is_red(tmp)) {         
                /* uncle node가 red이므로 recoloring */
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->rb_left;
            if(node == tmp) {               
                /* uncle node가 black이고 node가 parent의 left child인 경우 */
                tmp = node->rb_right;
                WRITE_ONCE(parent->rb_left, tmp);
                WRITE_ONCE(node->rb_right, parent);
                if(tmp)
                    rb_set_parent_color(tmp, parent, RB_BLACK);
                rb_set_parent_color(parent, node, RB_RED);
                parent = node;
                tmp = node->rb_left;
            }
            /* uncle node가 black이고 node가 parent의 right child인 경우 */
            WRITE_ONCE(gparent->rb_right, tmp);
            WRITE_ONCE(parent->rb_left, gparent);
            if(tmp) 
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            __rb_rotate_set_parents(gparent, parent, &root->rb_root, RB_RED);
            break;
        }
    }
}

void rb_insert_color_cached(struct rb_node *node, struct rb_root_cached *root, bool leftmost)
{
    if(leftmost) 
        root->rb_leftmost = node;

    printf("rb insert \n\r");
    __rb_insert(node, root);

}

void rb_erase_cached(struct rb_node *node, struct rb_root_cached *root)
{ 
    if(root->rb_leftmost == node)
        root->rb_leftmost = rb_next(node);
    //rb_erase(node, &root->rb_root);
}
