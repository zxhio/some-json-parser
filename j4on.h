// File:    j4on.h
// Author:  definezxh@163.com
// Date:    2019/04/08 00:50:20

#ifndef J4ON_H
#define J4ON_H

#include <stddef.h> // NULL, offsetof

struct slist {
    struct slist *breadth, *depth;
};

// #define offsetof(type, member) ((size_t) & ((type *)0)->member)

#define container_of(ptr, type, member)                                        \
    (type *)((char *)ptr - offsetof(type, member))

#define slist_entry(ptr, type, member) container_of(ptr, type, member)

#define slist_for_each(pos, head)                                              \
    for (pos = (head)->breadth; pos != NULL; pos = pos->breadth)

#define slist_init(list)                                                       \
    do {                                                                       \
        (list)->breadth = NULL;                                                \
        (list)->depth = NULL;                                                  \
    } while (0)

typedef enum {
    J4_NULL = 1,
    J4_FALSE,
    J4_TRUE,
    J4_NUMBER,
    J4_STRING,
    J4_ARRAY,
    J4_OBJECT
} value_type;

struct j4on_key {
    char *key;
    size_t k_len;
};

struct j4on_value {
    value_type j4_type;
    struct slist j4_list;
};

typedef struct j4on_literal {
    struct j4on_key j4_key;
    struct j4on_value j4_value;
} j4on_literal;

typedef struct j4on_number {
    struct j4on_key j4_key;
    struct j4on_value j4_value;
    double number;
} j4on_number;

typedef struct j4on_string {
    struct j4on_key j4_key;
    struct j4on_value j4_value;
    char *str;
    size_t s_len;
} j4on_string;

typedef struct j4on_array {
    struct j4on_key j4_key;
    struct j4on_value j4_value;
} j4on_array;

typedef struct j4on_object {
    struct j4on_key j4_key;
    struct j4on_value j4_value;
} j4on_object;

struct json {
    char *content;
};

void j4on_load(struct json *json, const char *filename);
void j4on_parse(struct slist **list, struct json *json);
char *j4on_format(struct slist *list, const char *json);

#endif // J4ON_H