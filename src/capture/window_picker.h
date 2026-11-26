#pragma once
#include <optional>
#include <string>

struct PickedWindow {
    int pid = 0;
    std::string desktopFile;
};

std::optional<PickedWindow> pickWindow();
