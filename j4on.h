// File:    j4on.h
// Author:  definezxh@163.com
// Date:    2019/07/11 23:17:55
// Desc:
//   Light weight json parser of C++ implementation.

#pragma once

#include <string>
#include <vector>
#include <any>
#include <memory>

#include <string.h>

namespace j4on {

class FmtBuffer {
  public:
    FmtBuffer()
        : usedBytes_(0), bufferSize_(23),
          buffer_(std::make_unique<char[]>(bufferSize_)){};

    char *begin() { return buffer_.get(); }

    void append(const char *str, size_t len) {
        resizeIfNedd(len);
        std::copy(str, str + len, buffer());
        usedBytes_ += len;
    }

    FmtBuffer &operator<<(const std::string &str) {
        append(str.data(), str.length());
        return *this;
    }

    FmtBuffer &operator<<(const char *str) {
        append(str, strlen(str));
        return *this;
    }

    FmtBuffer &operator<<(char ch) {
        append(&ch, 1);
        return *this;
    }

    void indent(size_t n) {
        for (int i = 0; i < n; ++i)
            *this << '\t';
    }

  private:
    char *buffer() { return buffer_.get() + usedBytes_; }

    void resizeIfNedd(size_t len) {
        size_t size = usedBytes_ + len;
        if (size < bufferSize_)
            return;

        bufferSize_ = std::max(size, bufferSize_ * 2);
        std::unique_ptr<char[]> newBuffer =
            std::make_unique<char[]>(bufferSize_);
        std::copy(begin(), begin() + usedBytes_, newBuffer.get());
        std::swap(newBuffer, buffer_);
    }

    size_t usedBytes_;
    size_t bufferSize_;
    std::unique_ptr<char[]> buffer_;
};

enum ValueType {
    kNull,
    kFalse,
    kTrue,
    kNumber,
    kString,
    kArray,
    kObject,
    kUnknown
};

const char *typeToString(ValueType type);

// General Value structure
class Value {
  public:
    Value() : type_(kUnknown) {}
    Value(ValueType type, std::any value) : type_(type), value_(value) {}
    ValueType type() const { return type_; }
    std::any getAnyValue() const { return value_; }
    void setAnyValue(std::any value) { value_ = value; }

  private:
    ValueType type_;
    std::any value_;
};

// Value.
class Literal {
  public:
    explicit Literal(const char *liteal, size_t n) : literal_(liteal, n) {}
    const std::string &getLiteral() const { return literal_; };

  private:
    std::string literal_;
};

class Number {
  public:
    Number(double number) : number_(number) {}
    double getNumber() const { return number_; }

  private:
    double number_;
};

class String {
  public:
    explicit String(std::string str) : str_(str) {}
    String(const char *str, size_t n) : str_(str, n) {}
    std::string getString() const { return str_; }

  private:
    std::string str_; // should be std::string not std::string_view
};

class Array {
  public:
    Array() {}
    Value operator[](size_t index) const { return values_[index]; }
    void add(Value v) { values_.push_back(v); }
    size_t size() const { return values_.size(); }

  private:
    std::vector<Value> values_;
};

class Object {
  public:
    Object(){};
    Object(std::pair<std::string, Value> member) : members_{member} {};

    std::pair<std::string, Value> operator[](size_t index) const {
        return members_[index];
    }

    // O(n)
    Value operator[](std::string &key) const {
        for (auto member : members_)
            if (key == member.first)
                return member.second;
        return Value();
    }

    void add(std::pair<std::string, Value> member) {
        members_.push_back(member);
    }

    size_t size() const { return members_.size(); }

  private:
    // <key, value>, use std::vector keep json order.
    // std::unordered_map<std::string, Value> members_;
    std::vector<std::pair<std::string, Value>> members_;
};

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

// string, number, true, false, null is leaf node.
// Parse JSON in the above order.
class J4onParser {
  public:
    J4onParser(const char *filename);

    // Start parse JSON.
    void parse();

    // Firse Value
    Value getRootValue() const { return *rootValue_.get(); }

    // get value by key.
    // if not found, return value with type is kUnkown.
    Value getValue(const std::string &key) const;

    // Traverse j4on parser tree.
    void traverse();

    // Debug.
    template <typename T> void check(T actual, T expect);
    template <typename T> void check(T actual, T expect, const char *msg);
    template <typename T>
    void check(bool t, T actual, T expect, const char *msg);
    template <typename T> void check(bool t, T actual, const char *expect);

  private:
    char *beginParse() { return &context_[index_]; }
    uint32_t getTokenIndex() const { return index_; }
    uint32_t getJsonLength() const { return context_.size(); }
    char getCurrToken() const { return context_[index_]; }
    char getNextToken();

    // Parse Value
    // @return is value. if it's not root value, we need take it to array or
    // object. when return as reference or pointer, value's lifetime will be end
    // at function execuation completion.
    Value parseValue();
    Value parseLiteral(const char *literal, ValueType type, size_t n);
    Value parseNumber();
    Value parseString();
    Value parseArray();
    Value praseObject();

    // Intermediate Parse part.
    // Some parts will be deleted later.
    void parseWhitespace();
    Value parseElement();
    void parseElements(Array &array);
    std::pair<std::string, Value> parseMember();
    void parseMembers(Object &obj);

    // Get value in values.
    Value getValueInArray(Value &value, const std::string &key) const;
    Value getValueInObject(Value &value, const std::string &key) const;

    // Intermediate traversing.
    // @return value depth.
    void traverseValue(Value &value, uint32_t depth);
    void traverseLiteral(Value &value, uint32_t depth);
    void traverseNumber(Value &value, uint32_t depth);
    void traverseString(Value &value, uint32_t depth);
    void traverseArray(Value &value, uint32_t depth);
    void traverseObject(Value &value, uint32_t depth);

    uint32_t index_;
    uint32_t row_;
    uint32_t column_;
    std::vector<char> context_;
    std::unique_ptr<Value> rootValue_;
    FmtBuffer formattedContext_;
};

} // namespace j4on
