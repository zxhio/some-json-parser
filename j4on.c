// File:    j4on.c
// Author:  definezxh@163.com
// Date:    2019/04/12 19:54:04

#include "j4on.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#define LOG(format, x)                                                         \
    fprintf(stderr, "%s:%d, " format " \n", __FILE__, __LINE__, x);

#define LOG_ERR(format, x)                                                     \
    do {                                                                       \
        LOG(format, x);                                                        \
        exit(0);                                                               \
    } while (0)

#define BREADTH_TYPE (J4_NULL | J4_FALSE | J4_TRUE | J4_NUMBER | J4_STRING)
#define DEPTH_TYPE (J4_ARRAY | J4_OBJECT)

#define J4ON_KEY_VALUE_INIT(j4on, key, value, type)                            \
    do {                                                                       \
        slist_init(&(j4on)->j4_value.j4_list);                                 \
        (j4on)->j4_key.key = (key)->key;                                       \
        (j4on)->j4_key.k_len = (key)->k_len;                                   \
        (j4on)->j4_value.j4_type = (type);                                     \
    } while (0)

struct json {
    char *content;
};

void read_file(struct json *json, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp)
        LOG_ERR("File %s open failed.", filename);

    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    json->content = (char *)malloc(len + 1);
    fread(json->content, sizeof(char), len, fp);
    json->content[len] = '\0';
}

void skip_whitespce(struct json *json) {
    char *p = json->content;
    while (*p == ' ' || *p == '\r' || *p == '\n' || *p == '\t')
        p++;
    json->content = p;
}

struct j4on_key *j4on_parse_key(struct json *json) {
    skip_whitespce(json);
    char *p = json->content;

    struct j4on_key *j4_key =
        (struct j4on_key *)malloc(sizeof(struct j4on_key));
    j4_key->k_len = 0;

    if (*p != '\"') { // key is ""
        j4_key->key = (char *)malloc(sizeof(char));
        *j4_key->key = '\0';
        return j4_key;
    }

    p++; // skip '\"'

    while (*p++ != '\"') {
        j4_key->k_len++;
    }

    j4_key->key = (char *)malloc(j4_key->k_len + 1);
    memmove(j4_key->key, ++json->content, j4_key->k_len);
    j4_key->key[j4_key->k_len] = '\0';
    json->content = p;
    return j4_key;
}

struct j4on_value *j4on_parse_literal(struct j4on_key *key, struct json *json,
                                      const char *literal, size_t len,
                                      value_type type) {
    const char *q = literal;
    char *p = json->content;
    for (size_t i = 0; i < len; i++) {
        if (*p++ != *q++)
            LOG_ERR("Expeted '%s'", literal);
    }

    json->content = p;

    j4on_literal *j4_literal = (j4on_literal *)malloc(sizeof(j4on_literal));
    J4ON_KEY_VALUE_INIT(j4_literal, key, j4_literal->j4_value, type);

    return &j4_literal->j4_value;
}

struct j4on_value *j4on_parse_number(struct j4on_key *key, struct json *json) {
    char *p = json->content;
    char *q = p;

    // sign
    if (*p == '-')
        p++;

    // integer
    if (*p == '0') {
        p++;
    } else if (isdigit(*p)) {
        while (isdigit(*p))
            p++;
    } else {
        LOG_ERR("Expected digit, actual '%c'", *p);
    }

    // fractional part
    if (*p == '.') {
        p++;
        while (isdigit(*p))
            p++;
    }

    // exponent part
    if (*p == 'e' || *p == 'E')
        p++;
    if (*p == '+' || *p == '-')
        p++;
    while (isdigit(*p))
        p++;

    // character to number parse completion
    json->content = p;

    // converting parsed string to number and keep the number in correct range
    double number = strtod(q, &p);
    if (errno == ERANGE && (number == HUGE_VAL || number == -HUGE_VAL)) {
        LOG("Numerical result out of range: %lf", number)
    }

    // handle new json node
    j4on_number *j4_number = (j4on_number *)malloc(sizeof(j4on_number));
    J4ON_KEY_VALUE_INIT(j4_number, key, j4_number->j4_value, J4_NUMBER);

    return &j4_number->j4_value;
}

struct j4on_value *j4on_prase_string(struct j4on_key *key, struct json *json) {
    char *p = json->content;
    p++; // skip '\"'
    while (*p != '\0') {
        if (isgraph(*p))
            p++;
        else if (*p == '\\') {
            p++;
            switch (*p) {
            case '\"':
                p++;
                break;
            case '\\':
                p++;
                break;
            case '/':
                p++;
                break;
            case 'b':
                p++;
                break;
            case 'f':
                p++;
                break;
            case 'n':
                p++;
                break;
            case 'r':
                p++;
                break;
            case 't':
                p++;
                break;
            default:
                LOG_ERR("Illegal escape character '\%c'.", *p);
                break;
            }
        } else {
            LOG_ERR("Illegal character '%c'.", *p);
        }
        if (*p == '\"') {
            p++;
            break;
        }
     }

    j4on_string *j4_string = (j4on_string *)malloc(sizeof(j4on_string));
    j4_string->s_len = p - json->content - 2;
    j4_string->str = (char *)malloc(j4_string->s_len + 1);
    memmove(j4_string->str, json->content + 1,
            j4_string->s_len); // in order to skip \", so + 1
    j4_string->str[j4_string->s_len] = '\0';
    J4ON_KEY_VALUE_INIT(j4_string, key, j4_string->j4_value, J4_STRING);

    json->content = p;

    return &j4_string->j4_value;
}

struct j4on_value *j4on_parse_value(struct j4on_key *key, struct json *json) {
    skip_whitespce(json);
    switch (*json->content) {
    case 'n':
        return j4on_parse_literal(key, json, "null", 4, J4_NULL);
    case 't':
        return j4on_parse_literal(key, json, "true", 4, J4_TRUE);
    case 'f':
        return j4on_parse_literal(key, json, "false", 5, J4_FALSE);
    case '\"':
        return j4on_prase_string(key, json);
    default:
        return j4on_parse_number(key, json);
    }
}

void j4on_parse(struct slist *head, struct json *json) {
    struct slist *list = head;
    struct j4on_key *key;
    struct j4on_value *value;
    while (*json->content != '\0') {
        // parse key value.
        key = j4on_parse_key(json);
        if (key->k_len != 0) {
            skip_whitespce(json);
            if (*json->content++ != ':')
                LOG_ERR("Expected '%c'", ':');
        }
        value = j4on_parse_value(key, json);

        // add the new json node(value/key) to list tail.
        list->breadth = &value->j4_list;
        list = list->breadth;

        // handle ','
        skip_whitespce(json);
        //        if (*json->content != '\0' && *json->content != ',')
        //            LOG_ERR("Expected '%c'", *json->content);
        json->content++; // skip ','
    }
}

int main() {
    struct json json;
    read_file(&json, "./test/foo.json");
    struct slist list, *pos;
    slist_init(&list);
    j4on_parse(&list, &json);

    struct j4on_value *entry;
    slist_for_each(pos, &list) {
        entry = slist_entry(pos, struct j4on_value, j4_list);
        printf("type: %s  ", value_type_stringify[entry->j4_type]);
        j4on_literal *lit = slist_entry(entry, j4on_literal, j4_value);
        printf("key: %zu %s\n", lit->j4_key.k_len, lit->j4_key.key);
    }
}