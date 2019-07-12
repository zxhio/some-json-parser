#include "j4on.hh"

#include <iostream>

void test_literal(const char *file) {
    j4on::J4onParser parser(file);
    parser.parse();

    j4on::Value v = parser.getRootValue();
    j4on::Literal lit = std::any_cast<j4on::Literal>(v.getAnyValue());

    std::cout << "type: " << j4on::typeToString(v.type()) << ", "
              << "literal: " << lit.getLiteral() << std::endl;
}

void test_null() { test_literal("./test/json/null.json"); }
void test_true() { test_literal("./test/json/true.json"); }
void test_false() { test_literal("./test/json/false.json"); }

void test_number() {
    j4on::J4onParser parser("./test/json/number.json");
    parser.parse();

    j4on::Value v = parser.getRootValue();
    j4on::Number number = std::any_cast<j4on::Number>(v.getAnyValue());

    std::cout << "type: " << j4on::typeToString(v.type()) << ", "
              << "number: " << number.getNumber() << std::endl;
}

int main() {
    test_null();
    test_true();
    test_false();
    test_number();
}