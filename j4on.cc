// File:    j4on.cc
// Author:  definezxh@163.com
// Date:    2019/07/11 23:35:32
// Desc:
//   Light weight json parser of C++ implementation.

#include "j4on.hh"

#include <cstdio>
#include <cassert>
#include <cmath>
#include <cstring>
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

// Intermediate Parse part.
template <typename T> void J4onParser::check(T expect, T actual) {
    check(expect == actual, expect, actual, "");
}

template <typename T>
void J4onParser::check(T expect, T actual, const char *msg) {
    check(expect == actual, expect, actual, msg);
}

template <typename T>
void J4onParser::check(bool t, T actual, const char *expect) {
    if (t)
        return;

    std::string line(beginParse() - column_, column_);
    std::cout << "Parse Failed at " << row_ + 1 << "," << column_ - 1 << '\n'
              << line << "\n Expect:\"" << expect << "\", actual: \"" << actual
              << "\"\n";
    exit(-1);
}

template <typename T>
void J4onParser::check(bool t, T expect, T actual, const char *msg) {
    if (t)
        return;

    std::string line(beginParse() - column_, column_);
    std::cout << "Parse Failed at " << row_ + 1 << "," << column_ - 1 << '\n'
              << line << "\n [" << msg << "] Expect:\"" << expect
              << "\", actual: \"" << actual << "\"\n";
    exit(-1);
}

char J4onParser::getNextToken() {
    check(getTokenIndex() <= getJsonLength(), getTokenIndex(), "Parse end");
    char ch = getCurrToken();
    index_++;
    column_++;
    if (ch == '\n') {
        row_++;
        column_ = 0;
    }
    return ch;
}

void J4onParser::parseWhitespace() {
    char ch = getCurrToken();
    if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t') {
        getNextToken();
        parseWhitespace();
    }
}

// Parse element.
void J4onParser::parse() {
    rootValue_ = std::make_unique<Value>(parseElement());
    check(getTokenIndex(), getJsonLength(), "Parse End"); // must be end
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
    if (getCurrToken() == ',') {
        getNextToken();
        parseElements(array);
    }
}

// ws string ws ':' element
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
    if (getCurrToken() == ',') {
        getNextToken();
        parseMembers(obj);
    }
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
    case '\"':
        return parseString();
    case '[':
        return parseArray();
    case '{':
        return praseObject();
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

Value J4onParser::parseNumber() {
    char *begin = beginParse();

    // sign
    if (getCurrToken() == '-')
        getNextToken();

    // integer
    check(isdigit(getCurrToken()), getCurrToken(), "Digit"); // 0 - 9
    if (getCurrToken() == '0') {                             // 0
        getNextToken();
    } else { // 1- 9
        while (isdigit(getCurrToken()))
            getNextToken();
    }

    // fractional part
    if (getCurrToken() == '.') {
        do {
            getNextToken();
        } while (isdigit(getCurrToken()));
    }

    // exponent part
    if (getCurrToken() == 'e' || getCurrToken() == 'E')
        getNextToken();
    if (getCurrToken() == '+' || getCurrToken() == '-')
        getNextToken();
    while (isdigit(getCurrToken()))
        getNextToken();

    // convert string to number.
    char *end = beginParse();
    double n = std::strtod(begin, &end);

    check(errno != ERANGE && (n != HUGE_VAL && n != -HUGE_VAL), n,
          "Legal number");

    Number number(n);
    std::any numberValue(number);
    // std::any numberValue(Number(n));  // ld error
    Value value(kNumber, numberValue);
    return value;
}

Value J4onParser::parseString() {
    std::string str;

    check(getNextToken(), '\"', "Parsing string begin");
    do {
        if (getCurrToken() == '\\') {
            switch (getNextToken()) {
            case '\"':
                str += '\"';
                break;
            case '\\':
                str += '\\';
                break;
            case '/':
                str += '/';
                break;
            case 'b':
                str += '\b';
                break;
            case 'f':
                str += '\f';
                break;
            case 'n':
                str += '\n';
                break;
            case 'r':
                str += '\r';
                break;
            case 't':
                str += '\t';
                break;
            default:
                // FIME: 4 hex digits.
                check(true, getCurrToken(), "Legal escape character");
            }
        } else if (getCurrToken() == '\"') {
            break;
        } else {
            str += getCurrToken();
        }
    } while (getNextToken());

    check(getNextToken(), '\"', "Parsing string end");

    String strValue(str);
    std::any v(strValue);
    Value value(kString, v);
    return value;
}

// '[' ws | elements ']'
Value J4onParser::parseArray() {
    check(getNextToken(), '[', "Parsing array begin");

    Array array;

    // parse ws or elements
    parseWhitespace();
    if (getCurrToken() != ']')
        parseElements(array);

    check(getNextToken(), ']', "Parsing array end");

    std::any v(array);
    Value value(kArray, v);
    return value;
}

// '{ ws | members '}'
Value J4onParser::praseObject() {
    check(getNextToken(), '{', "Parsing object begin");

    Object obj;

    // parse ws or members
    parseWhitespace();
    if (getCurrToken() != '}')
        parseMembers(obj);

    check(getNextToken(), '}', "Parsing object end");

    std::any v(obj);
    Value value(kObject, v);
    return value;
}

} // namespace j4on
