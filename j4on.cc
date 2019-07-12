// File:    j4on.cc
// Author:  definezxh@163.com
// Date:    2019/07/11 23:35:32
// Desc:
//   Light weight json parser of C++ implementation.

#include "j4on.hh"

#include <cstdio>
#include <cassert>
#include <iostream>

namespace j4on {

static const char *typesname[8] = {"null",   "false", "true",   "number",
                                   "string", "array", "object", "unknown"};

const char *typeToString(ValueType type) { return typesname[type]; }

J4onParser::J4onParser(const char *filename)
    : index_(0), row_(0), column_(0), length_(0) {

    FILE *fp = fopen(filename, "r");
    fseek(fp, 0, SEEK_END);
    length_ = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    context_ = std::make_unique<char[]>(length_ + 1);
    fread(context_.get(), sizeof(char), length_, fp);
    context_.get()[length_] = '\0';
    fclose(fp);
}

char J4onParser::getNextToken() {
    char ch = context_[index_++];
    column_++;
    if (ch == '\n' || ch == '\r') {
        row_++;
        column_ = 0;
    }
    return ch;
}

// Intermediate Parse part.
template <typename T> void J4onParser::check(T actual, T expect) {
    if (actual == expect)
        return;

    std::string line(beginParse() - column_, column_);
    std::cout << "Parse Failed at " << row_ + 1 << "," << column_ - 1 << '\n'
              << line << "\n  Expect: " << expect << ", actual: " << actual
              << std::endl;
    exit(-1);
}

void J4onParser::parseWhitespace() {
    char ch = getCurrToken();
    if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t') {
        getNextToken();
        parseWhitespace();
    }
}

// ws value ws
Value J4onParser::parseElement() {
    parseWhitespace();
    Value value = parseValue();
    parseWhitespace();
    return value;
}

// element ',' elements
void J4onParser::parseElements(Array &array) {
    Value value = parseElement();
    array.add(value);
    if (getNextToken() == ',')
        parseElements(array);
}

// ws string sw ':' element
std::pair<std::string_view, Value> J4onParser::parseMember() {
    parseWhitespace();
    Value key = parseString();
    parseWhitespace();

    assert(getNextToken() == ':');

    Value value = parseElement();

    String keyValue = std::any_cast<String>(key.getAnyValue());
    std::string_view keystr = keyValue.getString();
    std::pair<std::string_view, Value> member = std::make_pair(keystr, value);

    return member;
}

// member ',' members
void J4onParser::parseMembers(Object &obj) {
    std::pair<std::string_view, Value> member = parseMember();
    obj.add(member);
    if (getNextToken() == ',')
        parseMembers(obj);
}

// Parse element.
void J4onParser::parse() {
    rootValue_ = std::make_unique<Value>(parseElement());
}

Value J4onParser::parseValue() {
    char ch = getCurrToken();
    switch (ch) {
    case 'n':
        return parseLiteral("null", kNull, 4);
    case 'f':
        return parseLiteral("false", kFalse, 5);
    case 't':
        return parseLiteral("true", kTrue, 4);
    default:
        return parseNumber();
    }
}

Value J4onParser::parseLiteral(const char *literal, ValueType type, size_t n) {
    const char *p = literal;
    for (size_t i = 0; i < n; ++i)
        check(getNextToken(), p[i]);

    std::any literalValue(Literal(literal, n));
    Value value(type, literalValue);
    return value;
}

Value J4onParser::parseString() {
    /* test*/
    String strValue("test", 4);
    std::any v(strValue);
    Value value(kString, v);
    return value;
}

Value J4onParser::parseNumber() { return parseValue(); }

} // namespace j4on