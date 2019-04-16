// File:    slist.h
// Author:  definezxh@163.com
// Date:    2019/04/11 22:32:34

#ifndef SLIST_H
#define SLIST_H

#include <stdio.h>

struct slist {
    struct slist *breadth, *depth;
};

#define offsetof(type, member) ((size_t) & ((type *)0)->member)

#define container_of(ptr, type, member)                                        \
    (type *)((char *)ptr - offsetof(type, member))

#define slist_entry(ptr, type, member) container_of(ptr, type, member)

#define slist_for_each(pos, head)                                              \
    for (pos = (head)->breadth; pos != NULL; pos = pos->breadth)

void slist_init(struct slist *list) {
    list->breadth = NULL;
    list->depth = NULL;
}

#endif // SLIST_H