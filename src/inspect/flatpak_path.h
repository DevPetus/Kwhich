#pragma once
#include <optional>
#include <string>

std::optional<std::string> translateFlatpakPath(const std::string& sandboxPath, const std::string& appId);
