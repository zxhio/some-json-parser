//===- nextjson.h - JSON Parser ---------------------------------*- C++ -*-===//
//
/// \file
/// A C++17 lightweight Parser for JSON.
//
// Author:  zxh
// Date:    2020/03/16 13:58:56
//===----------------------------------------------------------------------===//

#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <any>
#include <string_view>

#include <cassert>
#include <cerrno>
#include <cmath>

// JSON structure as follows.

// JSON
//   - element
// element
//   - ws value ws
// elements
//   - element ',' elements
// value
//   - object
//   - array
//   - string
//   - number
//   - "true"
//   - "false"
//   - "null"
// array
//   - '[' ws ']'
//   - '[' elements ']'
// object
//   - '{' ws '}'
//   - '{ members '}'
// members
//   - member
//   - member ',' members
// member
//   - ws string ws ':' element

namespace nextjson {

// JSON's value type
enum ValueType : uint8_t {
    kUnknown,
    kNull,
    kFalse,
    kTrue,
    kNumber,
    kString,
    kArray,
    kObject
};

// Value type literal.
static const char *ValueTypeName[8] = {"unknown", "null",   "false", "true",
                                       "number",  "string", "array", "object"};

/// Stringify value type \p type .
inline const char *typeToString(ValueType type) { return ValueTypeName[type]; }

/// Universal Value structure.
/// A \c Value object hold \c type_ and the any type value \c value_ .
class Value {
  public:
    Value() : type_(kUnknown) {}
    Value(ValueType type, const std::any &value) : type_(type), value_(value) {}
    ValueType type() const { return type_; }
    const std::any &getAnyValue() const { return value_; }
    void setAnyValue(const std::any &value) { value_ = value; }

  private:
    ValueType type_;
    std::any value_;
};

/// Literal value. e.g. null, false, true.
class Literal {
  public:
    explicit Literal(const char *liteal, size_t n) : literal_(liteal, n) {}
    const std::string &getLiteral() const { return literal_; };

  private:
    std::string literal_;
};

/// Number value.
class Number {
  public:
    Number(double number) : number_(number) {}
    double getNumber() const { return number_; }

  private:
    double number_;
};

/// String value.
class String {
  public:
    explicit String(std::string str) : str_(str) {}
    String(const char *str, size_t n) : str_(str, n) {}
    std::string getString() const { return str_; }

  private:
    std::string str_; // should be std::string not std::string_view
};

/// Array value.
class Array {
  public:
    Array() {}
    Value operator[](size_t index) const { return values_[index]; }
    void add(Value v) { values_.push_back(v); }
    size_t size() const { return values_.size(); }

  private:
    std::vector<Value> values_;
};

/// Object value.
class Object {
  public:
    using member_t = std::pair<std::string, Value>;

    Object() {}
    Object(const member_t &member) : memberList_{member} {}

    const member_t &operator[](size_t index) const {
        return memberList_[index];
    }

    // O(n)
    Value operator[](std::string &key) const {
        for (auto member : memberList_)
            if (key == member.first)
                return member.second;
        return Value();
    }

    void add(const member_t &member) { memberList_.push_back(member); }

    size_t size() const { return memberList_.size(); }

  private:
    // <key, value>, use std::vector keep JSON order.
    std::vector<member_t> memberList_;
};

class FileStream {
  public:
    FileStream(const char *filename) : filename_(filename) {
        std::ifstream input(filename, std::ios::binary);
        std::vector<char> filedata((std::istreambuf_iterator<char>(input)),
                                   std::istreambuf_iterator<char>());
        std::swap(filedata, content_);
    }

    const char *data() const { return content_.data(); }
    size_t size() const { return content_.size(); }

  private:
    const char *filename_;
    std::vector<char> content_;
};

/// Wrapper of std::vector<char>
template <size_t N = 64> class Buffer {
  public:
    Buffer()
        : size_(0),
          capacity_(N - sizeof(size_t) * 2 - sizeof(char *)),
          ptr_(nullptr) {
        static_assert(N != 0 && ((N & (N - 1)) == 0), "Not Power of 2");
    }

    ~Buffer() {
        if (ptr_)
            std::free(ptr_);
    }
    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    char *begin() { return ptr_ ? ptr_ : data_; }
    char *end() { return begin() + size_; }
    const char *data() const { return ptr_ ? ptr_ : data_; }

    void append(const char *data, size_t n) {
        grow(n);
        std::copy(data, data + n, end());
        size_ += n;
    }

    void append(const char *data) {
        append(data, std::char_traits<char>::length(data));
    }
    void append(const std::string &str) { append(str.data(), str.length()); }
    void append(char ch) { append(&ch, 1); }

  private:
    void grow(size_t n) {
        size_t newSize = size_ + n;
        if (newSize < capacity_)
            return;

        // grow witg 1.5 size.
        capacity_ = std::max(newSize, capacity_ + (capacity_ + 1) / 2);
        char *p = static_cast<char *>(std::malloc(capacity_));
        std::copy(data(), data() + size_, p);
        std::swap(p, ptr_);
        std::free(p);
    }

    size_t size_;
    size_t capacity_;
    char *ptr_;
    char data_[N - sizeof(size_t) * 2 - sizeof(char *)];
};

using SmallBuffer = Buffer<64>;
using MeduimBuffer = Buffer<256>;
using LargeBuffer = Buffer<1024>;

class Formatter {
  public:
    const char *data() const { return buffer_.data(); }
    size_t size() const { return buffer_.size(); }
    void format(const Value &root) { formatValue(root, 0); }

  private:
    void formatIndent(uint32_t depth) {
        for (uint32_t i = 0; i < depth; ++i)
            buffer_.append('\t');
    }

    void formatValue(const Value &value, uint32_t depth) {
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
            return; /* do nothing. */
        }
    }
    void formatLiteral(const Value &value, uint32_t depth) {
        Literal literal = std::any_cast<Literal>(value.getAnyValue());
        buffer_.append(literal.getLiteral());
    }
    void formatNumber(const Value &value, uint32_t depth) {
        Number number = std::any_cast<Number>(value.getAnyValue());
        // replace std::to_string with snprintf("%.g");
        // in some case. eg: 1e-09, the result will be 0.0000.
        // better performance could use Grisu2/3 algorithm.
        char buf[12];
        snprintf(buf, sizeof(buf), "%.12g", number.getNumber());
        buffer_.append(buf);
    }
    void formatString(const Value &value, uint32_t depth) {
        String str = std::any_cast<String>(value.getAnyValue());
        buffer_.append('\"');
        buffer_.append(str.getString());
        buffer_.append('\"');
    }
    void formatArray(const Value &value, uint32_t depth) {
        Array arr = std::any_cast<Array>(value.getAnyValue());

        buffer_.append('[');
        if (arr.size() > 0)
            buffer_.append('\n');

        Value v;
        for (size_t i = 0; i < arr.size(); ++i) {
            v = arr[i];
            formatIndent(depth + 1);

            formatValue(v, depth + 1);

            if (i != arr.size() - 1) // last element.
                buffer_.append(",\n", 2);
            else
                buffer_.append('\n');
        }

        if (arr.size() > 0)
            formatIndent(depth);
        buffer_.append(']');
    }
    void formatObject(const Value &value, uint32_t depth) {
        Object obj = std::any_cast<Object>(value.getAnyValue());

        buffer_.append('{');
        if (obj.size() > 0)
            buffer_.append('\n');

        Value v;
        std::pair<std::string, Value> member;
        for (size_t i = 0; i < obj.size(); ++i) {
            member = obj[i];

            // print key
            formatIndent(depth + 1);
            buffer_.append('\"');
            buffer_.append(member.first);
            buffer_.append("\":", 2);

            // value.
            formatValue(member.second, depth + 1);

            // last element.
            if (i != obj.size() - 1)
                buffer_.append(",\n", 2);
            else
                buffer_.append('\n');
        }

        if (obj.size() > 0)
            formatIndent(depth);
        buffer_.append('}');
    }

    MeduimBuffer buffer_;
};

class Parser {
  public:
    Parser(const char *data, size_t n) : token_(0), view_(data, n) {}

    // Parse element.
    Value parse() {
        if (view_.size() == 0)
            return Value();
        Value value = parseElement();
        assert(view_.size() == token_);
        return value;
    }

  private:
    size_t token_;
    std::string_view view_;

    size_t length() const { return view_.size(); }
    const char *token() const { return view_.begin() + token_; }
    char peek() const { return *token(); }
    char next() { return *(view_.begin() + token_++); }

    void parseWhitespace() {
        char ch = peek();
        if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t')
            next();
        else
            return;
        parseWhitespace();
    }

    // ws value ws
    Value parseElement() {
        parseWhitespace();
        Value value = parseValue();
        parseWhitespace();

        return value;
    }

    // element ',' elements
    void parseElements(Array &array) {
        Value value = parseElement();
        array.add(value);
        if (peek() == ',') {
            next();
            parseElements(array);
        }
    }

    // ws string ws ':' element
    std::pair<std::string, Value> parseMember() {
        parseWhitespace();
        Value key = parseString();
        parseWhitespace();

        assert(next() == ':');

        Value value = parseElement();

        String keyValue = std::any_cast<String>(key.getAnyValue());
        std::string keystr = keyValue.getString();
        std::pair<std::string, Value> member = std::make_pair(keystr, value);

        return member;
    }

    // member ',' members
    void parseMembers(Object &obj) {
        std::pair<std::string, Value> member = parseMember();
        obj.add(member);
        if (peek() == ',') {
            next();
            parseMembers(obj);
        }
    }

    Value parseValue() {
        char ch = peek();
        switch (ch) {
        case 'n':
            return parseLiteral("null", ValueType::kNull, 4);
        case 'f':
            return parseLiteral("false", ValueType::kFalse, 5);
        case 't':
            return parseLiteral("true", ValueType::kTrue, 4);
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

    Value parseLiteral(const char *literal, ValueType type, size_t n) {
        const char *p = literal;
        for (size_t i = 0; i < n; ++i)
            assert(next() == p[i]);

        Value value(type, Literal(literal, n));
        return value;
    }

    Value parseNumber() {
        const char *begin = token();

        // sign
        if (peek() == '-')
            next();

        // integer
        // check(isdigit(peek()), peek(), "Digit"); // 0 - 9
        assert(isdigit(peek()) == true);
        if (peek() == '0') { // 0
            next();
        } else { // 1- 9
            while (isdigit(peek()))
                next();
        }

        // fractional part
        if (peek() == '.') {
            do {
                next();
            } while (isdigit(peek()));
        }

        // exponent part
        if (peek() == 'e' || peek() == 'E')
            next();
        if (peek() == '+' || peek() == '-')
            next();
        while (isdigit(peek()))
            next();

        // convert string to number.
        double n = std::strtod(begin, NULL);

        assert(errno != ERANGE && (n != HUGE_VAL && n != -HUGE_VAL));

        Value value(ValueType::kNumber, Number(n));
        return value;
    }

    Value parseString() {
        assert(next() == '\"');

        const char *begin = token();

        do {
            if (peek() == '\\') {
                switch (next()) {
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
                    // check(true, peek(), "Legal escape character");
                    ;
                }
            } else if (peek() == '\"') {
                break;
            } else {
                peek();
            }
        } while (next());

        size_t len = token() - begin;

        // check(next(), '\"', "Parsing string end");
        assert(next() == '\"');

        Value value(ValueType::kString, String(begin, len));
        return value;
    }

    // '[' ws | elements ']'
    Value parseArray() {
        assert(next() == '[');

        Array array;
        // parse ws or elements
        parseWhitespace();
        if (peek() != ']')
            parseElements(array);

        assert(next() == ']');

        Value value(ValueType::kArray, array);
        return value;
    }

    // '{ ws | members '}'
    Value praseObject() {
        assert(next() == '{');

        Object obj;

        // parse ws or members
        parseWhitespace();
        if (peek() != '}')
            parseMembers(obj);

        assert(next() == '}');

        Value value(ValueType::kObject, obj);
        return value;
    }
};

/// JSON
class Document {
  public:
    Document() : Document("", 0) {}
    Document(const char *data)
        : Document(data, std::char_traits<char>::length(data)) {}
    Document(const char *data, size_t n) : data_(data, n), parser_(data, n) {}
    Document(const FileStream &input) : Document(input.data(), input.size()) {}

    void parse() { rootValue_ = parser_.parse(); }

    void format() { formatter_.format(rootValue_); }

  private:
    std::string_view data_;
    Value rootValue_;
    Parser parser_;
    Formatter formatter_;
};

} // namespace nextjson