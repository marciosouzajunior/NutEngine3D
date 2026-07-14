#include "EditorUtils.h"

#include <cctype>
#include <fstream>
#include <sstream>

bool readTextFile(const std::string& path, std::string& outText) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::ostringstream stream;
    stream << file.rdbuf();
    outText = stream.str();
    return true;
}

bool writeTextFile(const std::string& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    return file.good();
}

bool fileExistsOnDisk(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return file.good();
}

std::string makeJsonKeyValue(const std::string& key, const std::string& value) {
    std::ostringstream out;
    out << "  \"" << escapeJsonString(key) << "\": \"" << escapeJsonString(value) << "\"";
    return out.str();
}

std::string escapeJsonString(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);

    for (char c : value) {
        switch (c) {
        case '\\': escaped += "\\\\"; break;
        case '"': escaped += "\\\""; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default: escaped.push_back(c); break;
        }
    }

    return escaped;
}

std::string trimCopy(const std::string& text) {
    size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }

    size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }

    return text.substr(start, end - start);
}

std::string trimAsciiWhitespace(const std::string& text) {
    return trimCopy(text);
}
