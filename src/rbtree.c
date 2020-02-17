#include "rbtree.h"

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

    //if()

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

void rb_insert_color_cached(struct rb_node *node, struct rb_root_cached *root, bool leftmost)
{
    if(leftmost) 
        root->rb_leftmost = node;

    //__rb_insert()
}

void rb_erase_cached(struct rb_node *node, struct rb_root_cached *root)
{ 
    if(root->rb_leftmost == node)
        root->rb_leftmost = rb_next(node);
    //rb_erase(node, &root->rb_root);
}
