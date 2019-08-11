// File:    j4on.hh
// Author:  definezxh@163.com
// Date:    2019/07/11 23:17:55
// Desc:
//   Light weight json parser of C++ implementation.

#pragma once

#include <string_view>
#include <string>
#include <vector>
#include <any>
#include <memory>

namespace j4on {

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
    std::string_view getLiteral() { return literal_; };

  private:
    std::string_view literal_;
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
    void traverse() const;

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
    void traverseValue(Value &value, uint32_t depth) const;
    void traverseLiteral(Value &value, uint32_t depth) const;
    void traverseNumber(Value &value, uint32_t depth) const;
    void traverseString(Value &value, uint32_t depth) const;
    void traverseArray(Value &value, uint32_t depth) const;
    void traverseObject(Value &value, uint32_t depth) const;

    uint32_t index_;
    uint32_t row_;
    uint32_t column_;
    std::vector<char> context_;
    std::unique_ptr<Value> rootValue_;
};

} // namespace j4on
