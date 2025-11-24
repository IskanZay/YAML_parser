#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <unordered_map>
#include <memory>
#include <variant>
#include <vector>
#include <iostream>

class ConfigValue {
public:
    using Number = int;
    using Dict = std::unordered_map<std::string, std::shared_ptr<ConfigValue>>;
    using Value = std::variant<Number, Dict>;

    ConfigValue() = default;
    ConfigValue(Number num) : value(num) {}
    ConfigValue(const Dict& dict) : value(dict) {}

    bool isNumber() const { return std::holds_alternative<Number>(value); }
    bool isDict() const { return std::holds_alternative<Dict>(value); }

    Number getNumber() const { return std::get<Number>(value); }
    const Dict& getDict() const { return std::get<Dict>(value); }

    void setValue(const Value& val) { value = val; }

private:
    Value value;
};

class ConfigParser {
public:
    ConfigParser();
    bool parseFile(const std::string& filename);
    void outputYAML(std::ostream& out) const;

private:
    std::string input;
    size_t pos;
    char currentChar;
    std::unordered_map<std::string, std::shared_ptr<ConfigValue>> constants;

    void advance();
    void skipWhitespace();
    void skipComment();

    std::shared_ptr<ConfigValue> parseValue();
    std::shared_ptr<ConfigValue> parseNumber();
    std::shared_ptr<ConfigValue> parseDict();
    std::string parseName();
    void parseConstant();
    std::shared_ptr<ConfigValue> parseConstantExpression();

    void outputYAMLValue(std::ostream& out, const std::shared_ptr<ConfigValue>& value, int indent = 0) const;

    bool expect(char expected);
    void error(const std::string& message);
};

#endif