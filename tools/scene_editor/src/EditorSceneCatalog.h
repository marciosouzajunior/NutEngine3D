#pragma once

#include <string>
#include <utility>
#include <vector>

std::vector<std::pair<std::string, std::string>> availableEditorScenes();
std::string defaultEditorScenePath();
bool isProtectedScenePath(const std::string& scenePath);
std::string sceneDisplayNameForPath(const std::string& scenePath);
std::vector<std::string> availableEditorMeshPaths();
bool isKnownEditorScenePath(const std::string& scenePath);
bool isValidSceneRelativePath(const std::string& scenePath);
std::string normalizeSceneRelativePath(const std::string& rawPath);
std::string slugifySceneName(const std::string& sceneName);
