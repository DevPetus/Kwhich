#pragma once
#include <string>

#include "proc_info.h"

struct SourceResult {
    std::string kind;    // Flatpak, AppImage, Snap, Native package, Manual install, Unknown
    std::string detail;  // app id, appimage path, package name, etc.
};

SourceResult detectSource(const ProcInfo& info);