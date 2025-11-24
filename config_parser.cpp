#include "config_parser.h"
#include <fstream>
#include <sstream>
#include <cctype>

ConfigParser::ConfigParser() : pos(0), currentChar(0) {}

bool ConfigParser::parseFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    input = buffer.str();
    file.close();

    pos = 0;
    if (!input.empty()) {
        currentChar = input[0];
    }

    try {
        while (pos < input.length()) {
            skipWhitespace();
            if (pos >= input.length()) break;

            if (currentChar == ';') {
                skipComment();
                continue;
            }

            // Look ahead to determine what we're parsing
            size_t savePos = pos;
            char saveChar = currentChar;

            std::string name = parseName();
            if (!name.empty()) {
                skipWhitespace();
                if (currentChar == '<' && pos + 1 < input.length() && input[pos + 1] == '-') {
                    // This is a constant definition
                    pos = savePos;
                    currentChar = saveChar;
                    parseConstant();
                }
                else {
                    // This is part of a dictionary
                    pos = savePos;
                    currentChar = saveChar;
                    parseDict(); // Parse the entire dictionary
                }
            }
            else {
                error("Unexpected character: " + std::string(1, currentChar));
            }
        }
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        return false;
    }
}

void ConfigParser::advance() {
    pos++;
    if (pos < input.length()) {
        currentChar = input[pos];
    }
    else {
        currentChar = 0;
    }
}

void ConfigParser::skipWhitespace() {
    while (pos < input.length() && std::isspace(static_cast<unsigned char>(currentChar))) {
        advance();
    }
}

void ConfigParser::skipComment() {
    while (pos < input.length() && currentChar != '\n') {
        advance();
    }
}

std::shared_ptr<ConfigValue> ConfigParser::parseValue() {
    skipWhitespace();

    if (currentChar == '{') {
        return parseDict();
    }
    else if (currentChar == '?') {
        return parseConstantExpression();
    }
    else if (std::isdigit(static_cast<unsigned char>(currentChar))) {
        return parseNumber();
    }
    else {
        error("Expected value, got: " + std::string(1, currentChar));
        return nullptr;
    }
}

std::shared_ptr<ConfigValue> ConfigParser::parseNumber() {
    std::string numberStr;

    // First digit must be 1-9
    if (currentChar >= '1' && currentChar <= '9') {
        numberStr += currentChar;
        advance();
    }
    else {
        error("Number must start with digit 1-9");
    }

    // Subsequent digits 0-9
    while (pos < input.length() && std::isdigit(static_cast<unsigned char>(currentChar))) {
        numberStr += currentChar;
        advance();
    }

    try {
        int value = std::stoi(numberStr);
        return std::make_shared<ConfigValue>(value);
    }
    catch (const std::exception&) {
        error("Invalid number: " + numberStr);
        return nullptr;
    }
}

std::shared_ptr<ConfigValue> ConfigParser::parseDict() {
    if (!expect('{')) return nullptr;

    ConfigValue::Dict dict;

    while (pos < input.length() && currentChar != '}') {
        skipWhitespace();
        if (currentChar == '}') break;

        std::string name = parseName();
        if (name.empty()) {
            error("Expected name in dictionary");
            break;
        }

        skipWhitespace();
        if (!expect('-')) break;
        if (!expect('>')) break;

        auto value = parseValue();
        if (!value) break;

        dict[name] = value;

        skipWhitespace();
        if (currentChar == '.') {
            advance();
        }
        else if (currentChar != '}') {
            error("Expected '.' or '}' after dictionary entry");
            break;
        }

        skipWhitespace();
    }

    if (!expect('}')) return nullptr;

    return std::make_shared<ConfigValue>(dict);
}

std::string ConfigParser::parseName() {
    std::string name;

    // First character must be a letter
    if (pos < input.length() && std::isalpha(static_cast<unsigned char>(currentChar))) {
        name += currentChar;
        advance();
    }
    else {
        return "";
    }

    // Subsequent characters can be letters or digits
    while (pos < input.length() &&
        (std::isalnum(static_cast<unsigned char>(currentChar)))) {
        name += currentChar;
        advance();
    }

    return name;
}

void ConfigParser::parseConstant() {
    std::string name = parseName();
    if (name.empty()) {
        error("Expected constant name");
        return;
    }

    skipWhitespace();
    if (!expect('<')) return;
    if (!expect('-')) return;

    auto value = parseValue();
    if (!value) return;

    constants[name] = value;
}

std::shared_ptr<ConfigValue> ConfigParser::parseConstantExpression() {
    if (!expect('?')) return nullptr;
    if (!expect('(')) return nullptr;

    std::string name = parseName();
    if (name.empty()) {
        error("Expected constant name in expression");
        return nullptr;
    }

    if (!expect(')')) return nullptr;

    auto it = constants.find(name);
    if (it == constants.end()) {
        error("Undefined constant: " + name);
        return nullptr;
    }

    return it->second;
}

bool ConfigParser::expect(char expected) {
    skipWhitespace();
    if (currentChar == expected) {
        advance();
        return true;
    }
    else {
        error("Expected '" + std::string(1, expected) + "', got '" + std::string(1, currentChar) + "'");
        return false;
    }
}

void ConfigParser::error(const std::string& message) {
    throw std::runtime_error(message + " at position " + std::to_string(pos));
}

void ConfigParser::outputYAML(std::ostream& out) const {
    // Output all constants first
    for (const auto& [name, value] : constants) {
        out << name << ": ";
        outputYAMLValue(out, value);
        out << std::endl;
    }
}

void ConfigParser::outputYAMLValue(std::ostream& out, const std::shared_ptr<ConfigValue>& value, int indent) const {
    if (!value) return;

    if (value->isNumber()) {
        out << value->getNumber();
    }
    else if (value->isDict()) {
        const auto& dict = value->getDict();
        if (dict.empty()) {
            out << "{}";
        }
        else {
            out << std::endl;
            for (const auto& [key, val] : dict) {
                out << std::string(indent + 2, ' ') << key << ": ";
                outputYAMLValue(out, val, indent + 2);
                out << std::endl;
            }
        }
    }
}