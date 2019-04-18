// File:    j4on.c
// Author:  definezxh@163.com
// Date:    2019/04/12 19:54:04

#include "j4on.h"

#include <stdio.h>
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

#define J4ON_VALUE_INIT(j4on, value, type)                                     \
    do {                                                                       \
        slist_init(&(j4on)->j4_value.j4_list);                                 \
        (j4on)->j4_value.j4_type = (type);                                     \
    } while (0)

const char *value_type_stringify[8] = {"unknown", "null",   "false", "true",
                                       "number",  "string", "array", "object"};

void j4on_load(struct json *json, const char *filename) {
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

struct j4on_value *j4on_parse_literal(struct json *json, const char *literal,
                                      size_t len, value_type type) {
    const char *q = literal;
    char *p = json->content;
    for (size_t i = 0; i < len; i++) {
        if (*p++ != *q++)
            LOG_ERR("Expeted '%s'", literal);
    }

    json->content = p;

    j4on_literal *j4_literal = (j4on_literal *)malloc(sizeof(j4on_literal));
    J4ON_VALUE_INIT(j4_literal, j4_literal->j4_value, type);

    return &j4_literal->j4_value;
}

struct j4on_value *j4on_parse_number(struct json *json) {
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
        LOG_ERR("Expected digit, actual '%', %s", json->content);
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
    J4ON_VALUE_INIT(j4_number, j4_number->j4_value, J4_NUMBER);

    return &j4_number->j4_value;
}

struct j4on_value *j4on_parse_string(struct json *json) {
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
    J4ON_VALUE_INIT(j4_string, j4_string->j4_value, J4_STRING);

    json->content = p;

    return &j4_string->j4_value;
}

struct j4on_value *j4on_parse_value(struct json *json) {
    skip_whitespce(json);
    switch (*json->content) {
    case 'n':
        return j4on_parse_literal(json, "null", 4, J4_NULL);
    case 't':
        return j4on_parse_literal(json, "true", 4, J4_TRUE);
    case 'f':
        return j4on_parse_literal(json, "false", 5, J4_FALSE);
    case '\"':
        return j4on_parse_string(json);
    default:
        return j4on_parse_number(json);
    }
}
