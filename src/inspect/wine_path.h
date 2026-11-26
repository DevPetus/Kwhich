#pragma once
#include <optional>
#include <string>

std::optional<std::string> translateWinePath(const std::string& windowsPath, const std::string& environBlob);
