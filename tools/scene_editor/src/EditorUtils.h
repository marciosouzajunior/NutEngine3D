#pragma once

#include <string>

bool readTextFile(const std::string& path, std::string& outText);
bool writeTextFile(const std::string& path, const std::string& text);
bool fileExistsOnDisk(const std::string& path);
std::string makeJsonKeyValue(const std::string& key, const std::string& value);
std::string escapeJsonString(const std::string& value);
std::string trimCopy(const std::string& text);
std::string trimAsciiWhitespace(const std::string& text);
