#include "dual_install.h"

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

/**
 * Runs a command, discarding its output, and reports whether it exited 0.
 * @param command shell command to run
 * @return true if the command ran and exited successfully
 */
bool commandSucceeds(const std::string& command) {
    return std::system((command + " >/dev/null 2>&1").c_str()) == 0;
}

}  // namespace

/**
 * Checks for a native-side install: a .desktop file with this id under a
 * standard system or user applications directory. This is the counterpart
 * to isFlatpakInstalled -- together they tell us whether both a native
 * package and a Flatpak of the same app are present.
 * @param desktopFile desktop file id, e.g. "com.spotify.Client"
 * @return true if a matching .desktop file is installed outside Flatpak's own tree
 */
bool isNativeDesktopFileInstalled(const std::string& desktopFile) {
    if (desktopFile.empty()) return false;

    if (fs::exists("/usr/share/applications/" + desktopFile + ".desktop")) return true;
    if (fs::exists("/usr/local/share/applications/" + desktopFile + ".desktop")) return true;

    const char* home = std::getenv("HOME");
    if (home && fs::exists(fs::path(home) / ".local/share/applications" / (desktopFile + ".desktop"))) {
        return true;
    }

    return false;
}

/**
 * Checks whether Flatpak has an app installed under this id. KWin's
 * desktopFile happens to match Flatpak's reverse-DNS app id convention, so
 * the same id serves as the lookup key for both sides of the check.
 * @param desktopFile desktop file id
 * @return true if `flatpak info <id>` resolves
 */
bool isFlatpakInstalled(const std::string& desktopFile) {
    if (desktopFile.empty()) return false;
    return commandSucceeds("flatpak info '" + desktopFile + "'");
}
