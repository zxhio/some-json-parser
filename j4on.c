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
#include <stdarg.h>

#define LOG(format, args...)                                                   \
    fprintf(stderr, "%s: %d, " format "\n", __func__, __LINE__, ##args);

#define LOG_EXPECT(cond, format, args...)                                      \
    if (!(cond)) {                                                             \
        LOG(format, args);                                                     \
        exit(-1);                                                              \
    }

#define J4ON_VALUE_INIT(j4on, value, type)                                     \
    do {                                                                       \
        slist_init(&(j4on)->j4_value.j4_list);                                 \
        (j4on)->j4_value.j4_type = (type);                                     \
    } while (0)

#define J4ON_LINK_VALUE(list, tmp_list, value)                                 \
    do {                                                                       \
        if (!(list).depth) {                                                   \
            (list).depth = &(value)->j4_list;                                  \
            tmp_list = (list).depth;                                           \
        } else {                                                               \
            tmp_list->breadth = &(value)->j4_list;                             \
            tmp_list = tmp_list->breadth;                                      \
        }                                                                      \
    } while (0)

#define J4ON_LINK_PAIR(list, tmp_list, value)                                  \
    do {                                                                       \
        j4on_pair *j4_pair = (j4on_pair *)malloc(sizeof(j4on_pair));           \
        J4ON_VALUE_INIT(j4_pair, j4_pair->j4_value, J4_PAIR);                  \
        J4ON_VALUE_INIT(j4_pair, j4_pair->j4_key.j4_value, J4_PAIR);           \
        j4_pair->j4_key.j4_value = *(key);                                     \
        j4_pair->j4_value = *(value);                                          \
        J4ON_LINK_VALUE(list, tmp_list, value);                                \
    } while (0)

const char *value_type_stringify[9] = {"UNKNOWN", "NULL",   "FALSE",
                                       "TRUE",    "NUMBER", "STRING",
                                       "ARRAY",   "OBJECT", "PAIR"};

void j4on_load(struct json *json, const char *filename) {
    FILE *fp = fopen(filename, "r");
    LOG_EXPECT(fp, "File %s open failed.", filename);

    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    json->content = (char *)malloc(len + 1);
    // avoid new line's diff in CRLF and LF
    memset(json->content, '\0', len + 1);
    fread(json->content, sizeof(char), len, fp);
    json->content[len] = '\0';
    fclose(fp);
}

// \TODO Fix free error, json->content arrived end after parse.
void j4on_free(struct json *json) { free(json->content); }

static void skip_whitespace(struct json *json) {
    char *p = json->content;
    while (*p == ' ' || *p == '\r' || *p == '\n' || *p == '\t')
        p++;
    json->content = p;
}

static void parse_whitespace(char **str) {
    char *p = *str;
    while (*p == ' ' || *p == '\r' || *p == '\n' || *p == '\t')
        p++;
    *str = p;
}

static int next_is_end_char(struct json *json, char ch) {
    skip_whitespace(json);
    if (*json->content == ch || *json->content == '\0')
        return 1;
    return 0;
}

static struct j4on_value *j4on_parse_literal(struct json *json,
                                             const char *literal, size_t len,
                                             value_type type) {
    const char *q = literal;
    char *p = json->content;
    for (size_t i = 0; i < len; i++)
        LOG_EXPECT(*p++ == *q++, "expeted '%s', actual '%.*s'", literal, 10,
                   json->content);

    json->content = p;

    j4on_literal *j4_literal = (j4on_literal *)malloc(sizeof(j4on_literal));
    J4ON_VALUE_INIT(j4_literal, j4_literal->j4_value, type);

    return &j4_literal->j4_value;
}

static struct j4on_value *j4on_parse_number(struct json *json) {
    char *p = json->content;
    char *q = p;

    // sign
    if (*p == '-')
        p++;

    // integer
    LOG_EXPECT(isdigit(*p), "Expected digit, actual '%.*s'", 10, json->content);

    if (*p == '0') {
        p++;
    } else if (isdigit(*p)) {
        while (isdigit(*p))
            p++;
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

    // converting parsed string to number and keep the number in correct range
    double number = strtod(q, &p);
    LOG_EXPECT(errno != ERANGE && (number != HUGE_VAL && number != -HUGE_VAL),
               "Numerical result out of range: '%lf' at '%.*s'", number, 24,
               json->content)

    // character to number parse completion
    json->content = p;

    // handle new json node
    j4on_number *j4_number = (j4on_number *)malloc(sizeof(j4on_number));
    J4ON_VALUE_INIT(j4_number, j4_number->j4_value, J4_NUMBER);

    return &j4_number->j4_value;
}

static struct j4on_value *j4on_parse_string(struct json *json) {
    char *p = json->content;
    LOG_EXPECT(*p == '\"', "Expected '\"', actual '%c' in string '%.*s'", *p,
               16, p);
    p++; // skip '\"'
    while (*p != '\0') {
        if (*p == '\\') {
            p++; // skip '\\'
            switch (*p) {
            case '\"':
                break;
            case '\\':
                break;
            case '/':
                break;
            case 'b':
                break;
            case 'f':
                break;
            case 'n':
                break;
            case 'r':
                break;
            case 't':
                break;
            default:
                LOG("Illegal escape character '\%c'.", *p);
                break;
            }
        } else if (*p == '\"') {
            break;
        }
        p++;
    }

    LOG_EXPECT(*p == '\"', "Expected '\"', actual '%c' in string '%.*s'", *p,
               16, p);
    p++; // skip '\"'

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

static struct j4on_value *j4on_parse_value(struct json *json);

static struct j4on_value *j4on_parse_array(struct json *json) {
    LOG_EXPECT(*json->content++ == '[', "expected '[', %.*s", 16,
               json->content);

    j4on_array *j4_array = (j4on_array *)malloc(sizeof(j4on_array));
    J4ON_VALUE_INIT(j4_array, j4_array->j4_value, J4_ARRAY);

    struct slist *list;
    struct j4on_value *value;
    while (*json->content != '\0') {
        skip_whitespace(json);
        if (*json->content == ']')
            break;

        value = j4on_parse_value(json);

        // link the new node by the list, and j4_array as the array list head.
        J4ON_LINK_VALUE(j4_array->j4_value.j4_list, list, value);

        skip_whitespace(json);
        if (*json->content == ',') {
            json->content++;
            LOG_EXPECT(!next_is_end_char(json, ']'), "expected ',', %.*s", 16,
                       json->content);
        } else if (!next_is_end_char(json, ']')) {
            LOG_EXPECT(*json->content == ',', "expected ',', %.*s", 16,
                       json->content);
            json->content++;
        } else {
            break;
        }
    }

    LOG_EXPECT(*json->content++ == ']', "expected ']', %.*s", 16,
               json->content);

    return &j4_array->j4_value;
}

static struct j4on_value *j4on_parse_object(struct json *json) {
    LOG_EXPECT(*json->content++ == '{', "expected '{', %.*s", 16,
               json->content);

    j4on_object *j4_object = (j4on_object *)malloc(sizeof(j4on_object));
    J4ON_VALUE_INIT(j4_object, j4_object->j4_value, J4_OBJECT);

    struct slist *list;
    struct j4on_value *key, *value;
    while (*json->content != '\0') {
        skip_whitespace(json);
        if (*json->content == '}')
            break;
        key = j4on_parse_string(json);

        skip_whitespace(json);
        LOG_EXPECT(*json->content++ == ':', "member expected ':', %.*s", 16,
                   json->content);

        value = j4on_parse_value(json);

        // linked the pair
        J4ON_LINK_PAIR(j4_object->j4_value.j4_list, list, value);

        skip_whitespace(json);
        if (*json->content == ',') {
            json->content++;
            LOG_EXPECT(!next_is_end_char(json, '}'), "expected ',', %.*s", 16,
                       json->content);
        } else if (!next_is_end_char(json, '}')) {
            LOG_EXPECT(*json->content == ',', "expected ',', %.*s", 16,
                       json->content);
            json->content++;
        } else {
            break;
        }
    }

    LOG_EXPECT(*json->content++ == '}', "expected '}', %.*s", 16,
               json->content);

    return &j4_object->j4_value;
}

static struct j4on_value *j4on_parse_value(struct json *json) {
    skip_whitespace(json);
    switch (*json->content) {
    case 'n':
        return j4on_parse_literal(json, "null", 4, J4_NULL);
    case 't':
        return j4on_parse_literal(json, "true", 4, J4_TRUE);
    case 'f':
        return j4on_parse_literal(json, "false", 5, J4_FALSE);
    case '\"':
        return j4on_parse_string(json);
    case '[':
        return j4on_parse_array(json);
    case '{':
        return j4on_parse_object(json);
    default:
        return j4on_parse_number(json);
    }
}

// linked the value abreast in first depth
void j4on_parse(struct slist *head, struct json *json) {
    struct j4on_value *value;
    struct slist *list = head;
    skip_whitespace(json);
    while (*json->content != '\0') {
        value = j4on_parse_value(json);
        list->breadth = &value->j4_list;
        list = list->breadth;
    }
}

void j4on_travel(struct slist *list) {
    if (!list)
        return;

    struct j4on_value *entry = slist_entry(list, struct j4on_value, j4_list);
    printf("type: %s\n", value_type_stringify[entry->j4_type]);
    j4on_travel(entry->j4_list.depth);
    j4on_travel(entry->j4_list.breadth);
}