#include "Json.h"
#include <cctype>
#include <cstdlib>

namespace nut {
namespace {

class JsonParser {
private:
    const std::string& m_text;
    size_t m_pos;
    std::string m_error;

public:
    explicit JsonParser(const std::string& text) : m_text(text), m_pos(0) {}

    bool parse(Json& outJson, std::string* error) {
        skipWhitespace();

        if (!parseValue(outJson)) {
            setExternalError(error);
            return false;
        }

        skipWhitespace();
        if (m_pos != m_text.size()) {
            m_error = "Unexpected characters after JSON value.";
            setExternalError(error);
            return false;
        }

        return true;
    }

private:
    void setExternalError(std::string* error) const {
        if (error) {
            *error = m_error;
        }
    }

    void skipWhitespace() {
        while (m_pos < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[m_pos]))) {
            ++m_pos;
        }
    }

    bool match(char expected) {
        skipWhitespace();
        if (m_pos < m_text.size() && m_text[m_pos] == expected) {
            ++m_pos;
            return true;
        }

        return false;
    }

    bool parseValue(Json& outJson) {
        skipWhitespace();

        if (m_pos >= m_text.size()) {
            m_error = "Unexpected end of JSON.";
            return false;
        }

        char c = m_text[m_pos];
        if (c == '{') return parseObject(outJson);
        if (c == '[') return parseArray(outJson);
        if (c == '"') return parseStringValue(outJson);
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parseNumber(outJson);
        if (startsWith("true")) return parseBool(outJson, true);
        if (startsWith("false")) return parseBool(outJson, false);
        if (startsWith("null")) return parseNull(outJson);

        m_error = "Unexpected JSON token.";
        return false;
    }

    bool startsWith(const char* word) const {
        size_t i = 0;
        while (word[i] != '\0') {
            if (m_pos + i >= m_text.size() || m_text[m_pos + i] != word[i]) {
                return false;
            }
            ++i;
        }

        return true;
    }

    bool parseNull(Json& outJson) {
        m_pos += 4;
        outJson = Json();
        return true;
    }

    bool parseBool(Json& outJson, bool value) {
        m_pos += value ? 4 : 5;
        outJson.type = Json::Type::Bool;
        outJson.boolValue = value;
        return true;
    }

    bool parseNumber(Json& outJson) {
        const char* start = m_text.c_str() + m_pos;
        char* end = nullptr;
        double value = std::strtod(start, &end);

        if (end == start) {
            m_error = "Invalid JSON number.";
            return false;
        }

        m_pos += static_cast<size_t>(end - start);
        outJson.type = Json::Type::Number;
        outJson.numberValue = value;
        return true;
    }

    bool parseString(std::string& outString) {
        if (!match('"')) {
            m_error = "Expected string.";
            return false;
        }

        outString.clear();
        while (m_pos < m_text.size()) {
            char c = m_text[m_pos++];
            if (c == '"') {
                return true;
            }

            if (c == '\\') {
                if (m_pos >= m_text.size()) {
                    m_error = "Invalid string escape.";
                    return false;
                }

                char escaped = m_text[m_pos++];
                if (escaped == '"' || escaped == '\\' || escaped == '/') {
                    outString.push_back(escaped);
                } else if (escaped == 'n') {
                    outString.push_back('\n');
                } else if (escaped == 't') {
                    outString.push_back('\t');
                } else if (escaped == 'r') {
                    outString.push_back('\r');
                } else {
                    m_error = "Unsupported string escape.";
                    return false;
                }
            } else {
                outString.push_back(c);
            }
        }

        m_error = "Unterminated string.";
        return false;
    }

    bool parseStringValue(Json& outJson) {
        std::string value;
        if (!parseString(value)) {
            return false;
        }

        outJson.type = Json::Type::String;
        outJson.stringValue = value;
        return true;
    }

    bool parseArray(Json& outJson) {
        if (!match('[')) {
            return false;
        }

        outJson.type = Json::Type::Array;
        outJson.arrayValue.clear();

        skipWhitespace();
        if (match(']')) {
            return true;
        }

        while (true) {
            Json item;
            if (!parseValue(item)) {
                return false;
            }

            outJson.arrayValue.push_back(item);

            if (match(']')) {
                return true;
            }

            if (!match(',')) {
                m_error = "Expected ',' or ']'.";
                return false;
            }
        }
    }

    bool parseObject(Json& outJson) {
        if (!match('{')) {
            return false;
        }

        outJson.type = Json::Type::Object;
        outJson.objectValue.clear();

        skipWhitespace();
        if (match('}')) {
            return true;
        }

        while (true) {
            std::string key;
            if (!parseString(key)) {
                return false;
            }

            if (!match(':')) {
                m_error = "Expected ':' after object key.";
                return false;
            }

            Json value;
            if (!parseValue(value)) {
                return false;
            }

            outJson.objectValue[key] = value;

            if (match('}')) {
                return true;
            }

            if (!match(',')) {
                m_error = "Expected ',' or '}'.";
                return false;
            }
        }
    }
};

} // namespace

Json::Json()
    : type(Type::Null), boolValue(false), numberValue(0.0) {}

bool Json::isNull() const { return type == Type::Null; }
bool Json::isBool() const { return type == Type::Bool; }
bool Json::isNumber() const { return type == Type::Number; }
bool Json::isString() const { return type == Type::String; }
bool Json::isArray() const { return type == Type::Array; }
bool Json::isObject() const { return type == Type::Object; }

bool Json::has(const std::string& key) const {
    return type == Type::Object && objectValue.find(key) != objectValue.end();
}

const Json& Json::get(const std::string& key) const {
    if (type != Type::Object) {
        return nullValue();
    }

    auto it = objectValue.find(key);
    if (it == objectValue.end()) {
        return nullValue();
    }

    return it->second;
}

const Json& Json::at(size_t index) const {
    if (type != Type::Array || index >= arrayValue.size()) {
        return nullValue();
    }

    return arrayValue[index];
}

size_t Json::size() const {
    if (type == Type::Array) {
        return arrayValue.size();
    }

    if (type == Type::Object) {
        return objectValue.size();
    }

    return 0;
}

std::string Json::asString(const std::string& defaultValue) const {
    return type == Type::String ? stringValue : defaultValue;
}

double Json::asNumber(double defaultValue) const {
    return type == Type::Number ? numberValue : defaultValue;
}

bool Json::asBool(bool defaultValue) const {
    return type == Type::Bool ? boolValue : defaultValue;
}

bool Json::parse(const std::string& text, Json& outJson, std::string* error) {
    JsonParser parser(text);
    return parser.parse(outJson, error);
}

const Json& Json::nullValue() {
    static Json value;
    return value;
}

} // namespace nut
