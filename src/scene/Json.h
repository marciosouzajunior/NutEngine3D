#pragma once

#include <map>
#include <string>
#include <vector>

namespace nut {

// Small JSON value/parser used by .nutscene files.
// It intentionally supports only the JSON features the scene loader needs.
class Json {
public:
    enum class Type {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object
    };

    Type type;
    bool boolValue;
    double numberValue;
    std::string stringValue;
    std::vector<Json> arrayValue;
    std::map<std::string, Json> objectValue;

    Json();

    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    bool has(const std::string& key) const;
    const Json& get(const std::string& key) const;
    const Json& at(size_t index) const;
    size_t size() const;

    std::string asString(const std::string& defaultValue = "") const;
    double asNumber(double defaultValue = 0.0) const;
    bool asBool(bool defaultValue = false) const;

    static bool parse(const std::string& text, Json& outJson, std::string* error = nullptr);

private:
    static const Json& nullValue();
};

} // namespace nut
