#include "j4on.h"

#include <iostream>

void test_literal(const char *file) {
    j4on::J4onParser parser(file);
    parser.parse();

    j4on::Value v = parser.getRootValue();
    j4on::Literal lit = std::any_cast<j4on::Literal>(v.getAnyValue());

    std::cout << "\nParse " << file << '\n';

    parser.format();
}

void test_null() { test_literal("./json/null.json"); }
void test_true() { test_literal("./json/true.json"); }
void test_false() { test_literal("./json/false.json"); }

void test_number() {
    j4on::J4onParser parser("./json/number.json");
    parser.parse();

    j4on::Value v = parser.getRootValue();
    j4on::Number number = std::any_cast<j4on::Number>(v.getAnyValue());

    std::cout << "\nParse ./json/number.json\n";

    parser.format();
}

void test_string() {
    j4on::J4onParser parser("./json/string.json");
    parser.parse();

    j4on::Value v = parser.getRootValue();
    j4on::String str = std::any_cast<j4on::String>(v.getAnyValue());

    std::cout << "\nParse ./json/string.json\n";

    parser.format();
}

void test_array() {
    j4on::J4onParser parser("./json/array.json");
    parser.parse();

    j4on::Value v = parser.getRootValue();
    j4on::Array arr = std::any_cast<j4on::Array>(v.getAnyValue());

    std::cout << "\nParse ./json/array.json\n";

    parser.format();
}

void test_object() {
    j4on::J4onParser parser("./json/object.json");
    parser.parse();

    j4on::Value v = parser.getRootValue();
    j4on::Object obj = std::any_cast<j4on::Object>(v.getAnyValue());

    std::cout << "\nParse test/json/object.json\n";

    parser.format();
}

void test_get_value() {
    j4on::J4onParser parser("./json/object.json");
    parser.parse();

    j4on::Value v = parser.getValue("latex-workshop.view.pdf.viewer");
    std::cout << j4on::typeToString(v.type()) << std::endl;
    j4on::String str = std::any_cast<j4on::String>(v.getAnyValue());
    std::cout << str.getString() << std::endl;

    v = parser.getValue("latex-workshop.iew.pdf.viewer");
    std::cout << j4on::typeToString(v.type()) << std::endl;

    v = parser.getValue("git.autofetch");
    std::cout << j4on::typeToString(v.type()) << std::endl;

    v = parser.getValue("tools");
    std::cout << j4on::typeToString(v.type()) << std::endl;

    v = parser.getValue("C_Cpp.clang_format_style");
    std::cout << j4on::typeToString(v.type()) << std::endl;
    str = std::any_cast<j4on::String>(v.getAnyValue());
    std::cout << str.getString() << std::endl;
}

void test_format_to_file() {
    j4on::J4onParser parser("./json/object.json");
    parser.parse();
    parser.format("formatted.json");
}

int main() {
    test_null();
    test_true();
    test_false();
    test_number();
    test_string();
    test_array();
    test_object();
    test_get_value();
    test_format_to_file();
}