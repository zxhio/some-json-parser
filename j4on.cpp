// File:    j4on.cpp
// Author:  definezxh@163.com
// Date:    2019/07/11 23:35:32
// Desc:
//   Light weight json parser of C++ implementation.

#include "j4on.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>

namespace j4on {

static const char *typesname[8] = {"null",   "false", "true",   "number",
                                   "string", "array", "object", "unknown"};

const char *typeToString(ValueType type) { return typesname[type]; }

J4onParser::J4onParser(const char *filename) : index_(0), row_(0), column_(0) {
    std::ifstream input(filename, std::ios::binary);
    std::vector<char> filedata((std::istreambuf_iterator<char>(input)),
                               std::istreambuf_iterator<char>());
    context_ = filedata;
}

// Intermediate Parse part.
template <typename T> void J4onParser::check(T actual, T expect) {
    check(expect == actual, actual, expect, "");
}

template <typename T>
void J4onParser::check(T actual, T expect, const char *msg) {
    check(expect == actual, actual, expect, msg);
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
void J4onParser::check(bool t, T actual, T expect, const char *msg) {
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
    if (context_.empty())
        return Value();

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
    check(getNextToken(), '\"', "Parsing string begin");

    char *begin = beginParse();

    do {
        if (getCurrToken() == '\\') {
            switch (getNextToken()) {
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
                // FIME: 4 hex digits.
                check(true, getCurrToken(), "Legal escape character");
            }
        } else if (getCurrToken() == '\"') {
            break;
        } else {
            getCurrToken();
        }
    } while (getNextToken());

    size_t len = beginParse() - begin;

    check(getNextToken(), '\"', "Parsing string end");

    Value value(kString, String(begin, len));
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

Value J4onParser::getValue(const std::string &key) const {
    Value value = getRootValue();

    if (value.type() == kArray)
        return getValueInArray(value, key);
    else if (value.type() == kObject)
        return getValueInObject(value, key);

    return Value(); // type is not json type.
}

Value J4onParser::getValueInArray(Value &value, const std::string &key) const {
    j4on::Array arr = std::any_cast<j4on::Array>(value.getAnyValue());

    size_t size = arr.size();
    Value v, retValue;
    for (size_t i = 0; i < size; ++i) {
        v = arr[i];
        if (v.type() == kArray)
            retValue = getValueInArray(v, key);
        else if (v.type() == kObject)
            retValue = getValueInObject(v, key);

        if (retValue.type() != kUnknown)
            return retValue;
    }

    return retValue; // type is kUnknown;
}

Value J4onParser::getValueInObject(Value &value, const std::string &key) const {
    j4on::Object obj = std::any_cast<j4on::Object>(value.getAnyValue());

    size_t size = obj.size();
    Value retValue;
    std::pair<std::string, Value> member;
    for (size_t i = 0; i < size; ++i) {
        member = obj[i];
        if (member.first == key) // match
            retValue = member.second;
        else if (member.second.type() == kArray)
            retValue = getValueInArray(member.second, key);
        else if (member.second.type() == kObject)
            retValue = getValueInObject(member.second, key);

        if (retValue.type() != kUnknown)
            return retValue;
    }

    return retValue; // type is kUnknown;
}

void J4onParser::format() {
    Value value = getRootValue();
    formatValue(value, 0);
    std::cout << formattedContext_.begin();
}

void J4onParser::format(const char *filename) {
    Value value = getRootValue();
    formatValue(value, 0);
    std::ofstream ofs(filename, std::ios::binary);
    ofs.write(formattedContext_.begin(), formattedContext_.length());
    ofs.close();
}

void J4onParser::formatValue(Value &value, uint32_t depth) {
    switch (value.type()) {
    case kNull:
    case kFalse:
    case kTrue:
        return formatLiteral(value, depth);
    case kNumber:
        return formatNumber(value, depth);
    case kString:
        return formatString(value, depth);
    case kArray:
        return formatArray(value, depth);
    case kObject:
        return formatObject(value, depth);
    case kUnknown:
        return; /* do nothing */
    }
}

void J4onParser::formatLiteral(Value &value, uint32_t depth) {
    Literal lit = std::any_cast<Literal>(value.getAnyValue());
    formattedContext_ << lit.getLiteral();
}

void J4onParser::formatNumber(Value &value, uint32_t depth) {
    Number number = std::any_cast<Number>(value.getAnyValue());
    // replace std::to_string with snprintf("%.g");
    // in some case. eg: 1e-09, the result will be 0.0000.
    // better performance could use Grisu2/3 algorithm.
    char buf[12];
    snprintf(buf, sizeof(buf), "%.12g", number.getNumber());
    formattedContext_ << buf;
}

void J4onParser::formatString(Value &value, uint32_t depth) {
    j4on::String str = std::any_cast<j4on::String>(value.getAnyValue());
    formattedContext_ << '\"' << str.getString() << '\"';
}

void J4onParser::formatArray(Value &value, uint32_t depth) {
    j4on::Array arr = std::any_cast<j4on::Array>(value.getAnyValue());

    formattedContext_ << '[';
    if (arr.size() > 0)
        formattedContext_ << '\n';

    Value v;
    for (size_t i = 0; i < arr.size(); ++i) {
        v = arr[i];
        formattedContext_.indent(depth + 1);

        formatValue(v, depth + 1);

        if (i != arr.size() - 1) // last element.
            formattedContext_ << ",\n";
        else
            formattedContext_ << '\n';
    }

    if (arr.size() > 0)
        formattedContext_.indent(depth);
    formattedContext_ << ']';
}

void J4onParser::formatObject(Value &value, uint32_t depth) {
    j4on::Object obj = std::any_cast<j4on::Object>(value.getAnyValue());

    formattedContext_ << '{';
    if (obj.size() > 0)
        formattedContext_ << '\n';

    Value v;
    std::pair<std::string, Value> member;
    for (size_t i = 0; i < obj.size(); ++i) {
        member = obj[i];

        // print key
        formattedContext_.indent(depth + 1);
        formattedContext_ << '\"' << member.first << "\": ";

        // value.
        formatValue(member.second, depth + 1);

        // last element.
        if (i != obj.size() - 1)
            formattedContext_ << ",\n";
        else
            formattedContext_ << '\n';
    }

    if (obj.size() > 0)
        formattedContext_.indent(depth);
    formattedContext_ << '}';
}

} // namespace j4on