#include "flatpak_path.h"

#include <filesystem>

#include "util/shell.h"

namespace fs = std::filesystem;

/**
 * Translates a path from inside a Flatpak sandbox's /app/ tree into the
 * real host path. Flatpak bind-mounts an app's installed "files" directory
 * onto /app/ inside the sandbox, so /proc/<pid>/exe (which reads back as
 * the sandbox's own view, not the host's) resolves to something like
 * "/app/lib/floorp/floorp" that doesn't exist on the host filesystem.
 * `flatpak info --show-location` gives the host directory for an app id,
 * which contains a "files" subdir matching that /app/ mount.
 * @param sandboxPath a path as read from inside the sandbox (e.g. exe or argv)
 * @param appId the Flatpak app id (KWin's desktopFile matches this convention)
 * @return the real host path, if it exists; nullopt if sandboxPath isn't
 *         under /app/, appId is empty, or the translated path can't be found
 */
std::optional<std::string> translateFlatpakPath(const std::string& sandboxPath, const std::string& appId) {
    if (appId.empty()) return std::nullopt;
    if (sandboxPath.rfind("/app/", 0) != 0) return std::nullopt;

    std::string hostAppDir = runCommand("flatpak info --show-location '" + appId + "'");
    if (hostAppDir.empty()) return std::nullopt;

    std::string hostPath = hostAppDir + "/files" + sandboxPath.substr(4);

    std::error_code ec;
    if (!fs::exists(hostPath, ec) || ec) return std::nullopt;
    return hostPath;
}
