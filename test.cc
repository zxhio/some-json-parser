#include "j4on.hh"

#include <iostream>

void test_literal(const char *file) {
    j4on::J4onParser parser(file);
    parser.parse();

    j4on::Value v = parser.getRootValue();
    j4on::Literal lit = std::any_cast<j4on::Literal>(v.getAnyValue());

    std::cout << "\nParse " << file << '\n';

    parser.traverse();
}

void test_null() { test_literal("./test/json/null.json"); }
void test_true() { test_literal("./test/json/true.json"); }
void test_false() { test_literal("./test/json/false.json"); }

void test_number() {
    j4on::J4onParser parser("./test/json/number.json");
    parser.parse();

    j4on::Value v = parser.getRootValue();
    j4on::Number number = std::any_cast<j4on::Number>(v.getAnyValue());
 
    std::cout << "\nParse ./test/json/number.json\n";

    parser.traverse();
}

void test_string() {
    j4on::J4onParser parser("./test/json/string.json");
    parser.parse();

    j4on::Value v = parser.getRootValue();
    j4on::String str = std::any_cast<j4on::String>(v.getAnyValue());

    std::cout << "\nParse ./test/json/string.json\n";

    parser.traverse();
}

void test_array() {
    j4on::J4onParser parser("./test/json/array.json");
    parser.parse();

    j4on::Value v = parser.getRootValue();
    j4on::Array arr = std::any_cast<j4on::Array>(v.getAnyValue());

    std::cout << "\nParse ./test/json/array.json\n";

    parser.traverse();
}

void test_object() {
    j4on::J4onParser parser("./test/json/object.json");
    parser.parse();

    j4on::Value v = parser.getRootValue();
    j4on::Object obj = std::any_cast<j4on::Object>(v.getAnyValue());

    std::cout << "\nParse test/json/object.json\n";

    parser.traverse();
}

int main() {
    test_null();
    test_true();
    test_false();
    test_number();
    test_string();
    test_array();
    test_object();
}