#include "../j4on.h"

void test_parse_value(const char *filename) {
    struct json json;
    j4on_load(&json, filename);
    struct slist list, *pos;
    slist_init(&list);
    j4on_parse(&list, &json);
    j4on_free(&json);
}

void test_parse_null() { test_parse_value("json/null.json"); }

void test_parse_false() { test_parse_value("json/false.json"); }

void test_parse_true() { test_parse_value("json/true.json"); }
void test_parse_number() { test_parse_value("json/number.json"); }
void test_parse_string() { test_parse_value("json/string.json"); }
void test_parse_array() { test_parse_value("json/array.json"); }

void test() {
    test_parse_null();
    test_parse_false();
    test_parse_true();
    test_parse_number();
    test_parse_string();
    test_parse_array();
}

int main() { void test(); }