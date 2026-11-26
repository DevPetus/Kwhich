#include "source_rules.h"

#include <array>
#include <optional>
#include <regex>

namespace {

bool startsWith(const std::string& s, const std::string& prefix) {
    return s.rfind(prefix, 0) == 0;
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

/**
 * Detects a Flatpak by its cgroup scope name (app-flatpak-<app.id>-...scope)
 * or, as a fallback, by the exe living under the Flatpak runtime's /app/ tree.
 * @param info proc info for the window's process
 * @return Flatpak result with the app id/path as detail, or nullopt
 */
std::optional<SourceResult> detectFlatpak(const ProcInfo& info) {
    static const std::regex cgroupPattern(R"(app-flatpak-([^-]+(?:\.[^-]+)*)-)");
    std::smatch match;
    if (std::regex_search(info.cgroup, match, cgroupPattern)) {
        return SourceResult{"Flatpak", match[1].str()};
    }
    if (startsWith(info.exe, "/app/")) {
        return SourceResult{"Flatpak", info.exe};
    }
    return std::nullopt;
}

/**
 * Detects an AppImage: exe runs from a FUSE mount under /tmp/.mount_*, and
 * the process still has the APPIMAGE env var AppImage's runtime sets.
 * @param info proc info for the window's process
 * @return AppImage result with the .AppImage path as detail, or nullopt
 */
std::optional<SourceResult> detectAppImage(const ProcInfo& info) {
    if (!startsWith(info.exe, "/tmp/.mount_")) return std::nullopt;
    auto appimagePath = getEnvVar(info.environ, "APPIMAGE");
    if (!appimagePath) return std::nullopt;
    return SourceResult{"AppImage", *appimagePath};
}

/**
 * Detects a Snap by the exe living under /snap/.
 * @param info proc info for the window's process
 * @return Snap result, or nullopt
 */
std::optional<SourceResult> detectSnap(const ProcInfo& info) {
    if (!startsWith(info.exe, "/snap/")) return std::nullopt;
    return SourceResult{"Snap", info.exe};
}

/**
 * Detects a distro-packaged binary by exe living anywhere under /usr/,
 * which rpm/dpkg own regardless of subdir (bin, lib, libexec, share/...).
 * /usr/local/ is excluded since that's the by-hand install convention.
 * The actual package name is resolved by the caller (shelling out to
 * rpm/dpkg), since that's a system lookup, not a /proc fact.
 * @param info proc info for the window's process
 * @return Native package result with the exe path as detail, or nullopt
 */
std::optional<SourceResult> detectNativePackage(const ProcInfo& info) {
    if (startsWith(info.exe, "/usr/") && !startsWith(info.exe, "/usr/local/")) {
        return SourceResult{"Native package", info.exe};
    }
    return std::nullopt;
}

/**
 * Detects a by-hand install: exe under /opt, /usr/local, or a user-local
 * prefix like ~/.local, none of which a package manager owns.
 * @param info proc info for the window's process
 * @return Manual install result with the exe path as detail, or nullopt
 */
std::optional<SourceResult> detectManualInstall(const ProcInfo& info) {
    static const std::array<std::string, 2> manualDirs = {"/opt/", "/.local/"};
    for (const auto& dir : manualDirs) {
        if (contains(info.exe, dir)) return SourceResult{"Manual install", info.exe};
    }
    if (startsWith(info.exe, "/usr/local/")) return SourceResult{"Manual install", info.exe};
    return std::nullopt;
}

}  // namespace

/**
 * Runs the source detectors in order and returns the first match. Order is
 * most-specific first (Flatpak/AppImage/Snap) before the broad native-path
 * check, so appending a new detector later is just one more line here.
 * @param info proc info for the window's process
 * @return first matching SourceResult, or "Unknown" if nothing matched
 */
SourceResult detectSource(const ProcInfo& info) {
    if (auto r = detectFlatpak(info)) return *r;
    if (auto r = detectAppImage(info)) return *r;
    if (auto r = detectSnap(info)) return *r;
    if (auto r = detectNativePackage(info)) return *r;
    if (auto r = detectManualInstall(info)) return *r;
    return SourceResult{"Unknown", info.exe};
}