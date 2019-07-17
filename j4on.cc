// File:    j4on.cc
// Author:  definezxh@163.com
// Date:    2019/07/11 23:35:32
// Desc:
//   Light weight json parser of C++ implementation.

#include "j4on.hh"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>

namespace j4on {

static const char *typesname[8] = {"null",   "false", "true",   "number",
                                   "string", "array", "object", "unknown"};

const char *typeToString(ValueType type) { return typesname[type]; }

static void printWhitespace(uint32_t n) {
    for (uint32_t i = 0; i < n; ++i)
        std::cout << '\t';
}

J4onParser::J4onParser(const char *filename) : index_(0), row_(0), column_(0) {
    std::ifstream input(filename, std::ios::binary);
    std::vector<char> filedata((std::istreambuf_iterator<char>(input)),
                               std::istreambuf_iterator<char>());
    context_ = filedata;
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
std::pair<std::string, Value> J4onParser::parseMember() {
    parseWhitespace();
    Value key = parseString();
    parseWhitespace();

    assert(getNextToken() == ':');

    Value value = parseElement();

    String keyValue = std::any_cast<String>(key.getAnyValue());
    std::string keystr = keyValue.getString();
    std::pair<std::string, Value> member = std::make_pair(keystr, value);

    return member;
}

// member ',' members
void J4onParser::parseMembers(Object &obj) {
    std::pair<std::string, Value> member = parseMember();
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

    Value value(type, Literal(literal, n));
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

    Value value(kNumber, Number(n));
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

    Value value(kString, String(str));
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

    Value value(kArray, array);
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

    Value value(kObject, obj);
    return value;
}

void J4onParser::traverse() const {
    Value value = getRootValue();
    traverseValue(value, 0);
    std::cout << '\n';
}

void J4onParser::traverseValue(Value &value, uint32_t depth) const {
    switch (value.type()) {
    case kNull:
    case kFalse:
    case kTrue:
        return traverseLiteral(value, depth);
    case kNumber:
        return traverseNumber(value, depth);
    case kString:
        return traverseString(value, depth);
    case kArray:
        return traverseArray(value, depth);
    case kObject:
        return traverseObject(value, depth);
    case kUnknown:
        /* do nothing */;
    }
}

void J4onParser::traverseLiteral(Value &value, uint32_t depth) const {
    Literal lit = std::any_cast<Literal>(value.getAnyValue());
    std::cout << lit.getLiteral();
}

void J4onParser::traverseNumber(Value &value, uint32_t depth) const {
    Number number = std::any_cast<Number>(value.getAnyValue());
    std::cout << number.getNumber();
}

void J4onParser::traverseString(Value &value, uint32_t depth) const {
    j4on::String str = std::any_cast<j4on::String>(value.getAnyValue());
    std::cout << '\"' << str.getString() << '\"';
}

void J4onParser::traverseArray(Value &value, uint32_t depth) const {
    j4on::Array arr = std::any_cast<j4on::Array>(value.getAnyValue());

    std::cout << "[";
    if (arr.size() > 0)
        std::cout << '\n';

    Value v;
    for (size_t i = 0; i < arr.size(); ++i) {
        v = arr[i];
        printWhitespace(depth + 1);

        traverseValue(v, depth + 1);

        if (i != arr.size() - 1) // last element.
            std::cout << ",\n";
        else
            std::cout << '\n';
    }

    if (arr.size() > 0)
        printWhitespace(depth);
    std::cout << "]";
}

void J4onParser::traverseObject(Value &value, uint32_t depth) const {
    j4on::Object obj = std::any_cast<j4on::Object>(value.getAnyValue());

    std::cout << "{";
    if (obj.size() > 0)
        std::cout << '\n';

    Value v;
    std::pair<std::string, Value> member;
    for (size_t i = 0; i < obj.size(); ++i) {
        member = obj[i];

        // print key
        printWhitespace(depth + 1);
        std::cout << '\"' << member.first << "\": ";

        // value.
        traverseValue(member.second, depth + 1);

        // last element.
        if (i != obj.size() - 1)
            std::cout << ",\n";
        else
            std::cout << '\n';
    }

    if (obj.size() > 0)
        printWhitespace(depth);
    std::cout << "}";
}

} // namespace j4on