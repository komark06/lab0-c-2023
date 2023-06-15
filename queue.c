#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

static inline element_t *new_element(const char *s)
{
    if (!s)
        return NULL;
    element_t *ne = malloc(sizeof(*ne));
    if (!ne)
        return NULL;
    size_t len = strlen(s);
    ne->value = malloc(len + 1);
    if (!ne->value) {
        free(ne);
        return NULL;
    }
    memcpy(ne->value, s, len);
    ne->value[len] = '\0';
    return ne;
}

static inline element_t *remove_element(element_t *obj,
                                        char *sp,
                                        size_t bufsize)
{
    if (!obj)
        return NULL;
    if (sp && bufsize != 0) {
        size_t len = strlen(obj->value);
        if (len > bufsize - 1)
            len = bufsize - 1;
        memcpy(sp, obj->value, len);
        sp[len] = '\0';
    }
    list_del(&obj->list);
    return obj;
}

static struct list_head *merge_two_queue(struct list_head *L1,
                                         struct list_head *L2)
{
    struct list_head *head = NULL, **ptr = &head;
    for (struct list_head **cur = NULL; L1 && L2; *cur = (*cur)->next) {
        if (strcmp(container_of(L1, element_t, list)->value,
                   container_of(L2, element_t, list)->value) >= 0)
            cur = &L2;
        else
            cur = &L1;
        *ptr = *cur;
        ptr = &(*ptr)->next;
    }
    *ptr = (struct list_head *) (void *) ((uintptr_t) (void *) L1 |
                                          (uintptr_t) (void *) L2);
    return head;
}

static struct list_head *merge_sort(struct list_head *head)
{
    if (!head || !head->next)
        return head;
    struct list_head *slow = head, *fast = head->next;
    for (; fast && fast->next; fast = fast->next->next, slow = slow->next)
        ;
    fast = slow->next;
    slow->next = NULL;
    return merge_two_queue(merge_sort(head), merge_sort(fast));
}

/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *q = malloc(sizeof(*q));
    if (!q)
        return NULL;
    INIT_LIST_HEAD(q);
    return q;
}

/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    if (!l)
        return;
    element_t *cur, *safe;
    list_for_each_entry_safe (cur, safe, l, list) {
        q_release_element(cur);
    }
    free(l);
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *ne = new_element(s);
    if (!ne)
        return false;
    list_add(&ne->list, head);
    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *ne = new_element(s);
    if (!ne)
        return false;
    list_add_tail(&ne->list, head);
    return true;
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    return remove_element(list_first_entry(head, element_t, list), sp, bufsize);
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    return remove_element(list_last_entry(head, element_t, list), sp, bufsize);
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;
    struct list_head *cur;
    int len = 0;
    list_for_each (cur, head) {
        len++;
    }
    return len;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    if (!head || list_empty(head))
        return false;
    struct list_head *tail = head->prev, *fast = head->next, *slow = fast;
    while (fast != tail && fast->next != tail) {
        fast = fast->next->next;
        slow = slow->next;
    }
    list_del(slow);
    q_release_element(container_of(slow, element_t, list));
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    if (!head)
        return false;
    struct list_head *cur = head->next;
    while (cur != head) {
        if (cur->next != head) {
            struct list_head *safe = cur->next;
            element_t *c = container_of(cur, element_t, list),
                      *n = container_of(cur->next, element_t, list);
            if (!strcmp(c->value, n->value)) {
                do {
                    struct list_head *next = n->list.next;
                    list_del(&n->list);
                    q_release_element(n);
                    if (next == head)
                        break;
                    n = container_of(next, element_t, list);
                } while (!strcmp(c->value, n->value));
                safe = cur->next;
                list_del(&c->list);
                q_release_element(c);
            }
            cur = safe;
        }
    }
    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    if (!head)
        return;
    struct list_head **cur = &head->next;
    for (; *cur != head && (*cur)->next != head; cur = &(*cur)->next->next) {
        struct list_head *first = *cur, *second = first->next;
        first->next = second->next;
        second->next = first;
        second->prev = first->prev;
        first->prev = second;
        *cur = second;
    }
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head)
        return;
    struct list_head *cur, *safe;
    list_for_each_safe (cur, safe, head) {
        list_move(cur, head);
    }
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    if (!head || k <= 1)
        return;
    struct list_head *cur, *safe, *tmp = head;
    LIST_HEAD(dummy);
    int count = 0;
    list_for_each_safe (cur, safe, head) {
        count++;
        if (count == k) {
            list_cut_position(&dummy, tmp, cur);
            q_reverse(&dummy);
            list_splice_init(&dummy, tmp);
            count = 0;
            tmp = safe->prev;
        }
    }
}

/* Sort elements of queue in ascending order */
void q_sort(struct list_head *head)
{
    if (!head || head->next == head->prev)
        return;
    head->prev->next = NULL;
    head->next = merge_sort(head->next);
    struct list_head *ptr = head;
    for (; ptr->next; ptr = ptr->next)
        ptr->next->prev = ptr;
    ptr->next = head;
    head->prev = ptr;
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    if (!head)
        return 0;
    int len = 0;
    struct list_head *cur = head->prev;
    while (cur != head) {
        len++;
        if (cur->prev == head)
            break;
        element_t *c = container_of(cur, element_t, list),
                  *p = container_of(cur->prev, element_t, list);
        if (strcmp(c->value, p->value) > 0) {
            list_del(&p->list);
            q_release_element(p);
            len--;
        } else {
            cur = cur->prev;
        }
    }
    return len;
}

/* Merge all the queues into one sorted queue, which is in ascending order */
int q_merge(struct list_head *head)
{
    if (!head || list_empty(head))
        return 0;
    queue_contex_t *fir = container_of(head->next, queue_contex_t, chain);
    if (head->next == head->prev)
        return fir->size;
    for (struct list_head *cur = head->next->next; cur != head;
         cur = cur->next) {
        queue_contex_t *que = container_of(cur, queue_contex_t, chain);
        list_splice_init(que->q, fir->q);
        que->size = 0;
    }
    q_sort(fir->q);
    fir->size = q_size(fir->q);
    return fir->size;
}
