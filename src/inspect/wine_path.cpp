#include "wine_path.h"

#include <cctype>
#include <cstdlib>
#include <filesystem>

#include "proc_info.h"

namespace fs = std::filesystem;

namespace {

/**
 * Recognizes a Windows-style drive path like "C:\..." or "C:/...".
 * @param path candidate argv entry
 * @return true if it starts with a drive letter and ':'
 */
bool looksLikeWindowsPath(const std::string& path) {
    return path.size() > 2 && std::isalpha(static_cast<unsigned char>(path[0])) && path[1] == ':' &&
           (path[2] == '\\' || path[2] == '/');
}

}  // namespace

/**
 * Translates a Windows-style path from a Wine process's argv (e.g.
 * "C:\Program Files (x86)\App\thing.exe") into the real host path, via
 * Wine's dosdevices drive-letter symlinks. Uses the process's own
 * WINEPREFIX env var when present -- falling back to ~/.wine, Wine's own
 * default -- so it resolves to the exact prefix that process actually runs
 * under rather than guessing a single global prefix.
 * @param windowsPath a path as it appears in a Wine process's argv
 * @param environBlob that process's raw /proc/<pid>/environ, for WINEPREFIX
 * @return the real host path, if it exists; nullopt if not a Windows path,
 *         or the prefix/translated path can't be found
 */
std::optional<std::string> translateWinePath(const std::string& windowsPath, const std::string& environBlob) {
    if (!looksLikeWindowsPath(windowsPath)) return std::nullopt;

    std::string prefix;
    if (auto winePrefix = getEnvVar(environBlob, "WINEPREFIX")) {
        prefix = *winePrefix;
    } else if (const char* home = std::getenv("HOME")) {
        prefix = std::string(home) + "/.wine";
    } else {
        return std::nullopt;
    }
    while (!prefix.empty() && prefix.back() == '/') prefix.pop_back();

    char driveLetter = static_cast<char>(std::tolower(static_cast<unsigned char>(windowsPath[0])));
    std::string relative = windowsPath.substr(2);  // drop "C:", keep the leading separator
    for (char& c : relative) {
        if (c == '\\') c = '/';
    }

    std::string hostPath = prefix + "/dosdevices/" + driveLetter + ":" + relative;

    std::error_code ec;
    if (!fs::exists(hostPath, ec) || ec) return std::nullopt;
    return hostPath;
}
