#include <QApplication>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "capture/window_picker.h"
#include "inspect/dual_install.h"
#include "inspect/file_info.h"
#include "inspect/proc_info.h"
#include "inspect/process_tree.h"
#include "inspect/source_rules.h"
#include "ui/inspector_panel.h"

namespace {

/**
 * Runs a shell command and captures its stdout.
 * @param command shell command to run; stderr is discarded
 * @return trimmed stdout, or empty string if the command failed to run
 */
std::string runCommand(const std::string& command) {
    std::array<char, 256> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen((command + " 2>/dev/null").c_str(), "r"), pclose);
    if (!pipe) return {};
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    return result;
}

/**
 * Resolves the distro package owning an executable, trying rpm then dpkg.
 * @param exePath absolute path to the executable
 * @return package name, or empty string if neither tool claims it
 */
std::string resolvePackageName(const std::string& exePath) {
    if (exePath.empty()) return {};

    std::string viaRpm = runCommand("rpm -qf '" + exePath + "'");
    if (!viaRpm.empty() && viaRpm.find("not owned") == std::string::npos) {
        return viaRpm;
    }

    std::string viaDpkg = runCommand("dpkg -S '" + exePath + "'");
    if (!viaDpkg.empty()) {
        auto colon = viaDpkg.find(':');
        if (colon != std::string::npos) return viaDpkg.substr(0, colon);
    }

    return {};
}

}  // namespace

/**
 * M4 spike: with no args, asks KWin to let the user click a window and
 * shows a Qt Widgets panel with that window's process info, detected
 * source, and the M4 extras (install date, dual-install note, process
 * tree). With a pid argument, skips the click (M0's path) for quick
 * testing against a known process -- the desktopFile-dependent dual-install
 * check is unavailable in that path, since there's no window to ask KWin about.
 * Meant to be launched fresh per invocation (e.g. via a .desktop-file-bound
 * global shortcut), not run as a persistent background process.
 * @param argc 1 (click a window) or 2 (argv[1] is a pid)
 * @param argv argv[1], if present, is the pid to inspect
 * @return Qt event loop's exit code on success, 1 on bad usage, a
 *         cancelled pick, or an unreadable/missing process
 */
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("Kwhich");

    if (argc > 2) {
        std::cerr << "usage: kwhich-cli [pid]\n";
        return 1;
    }

    int pid = 0;
    std::string desktopFile;
    if (argc == 2) {
        pid = std::atoi(argv[1]);
        if (pid <= 0) {
            std::cerr << "not a valid pid: " << argv[1] << "\n";
            return 1;
        }
    } else {
        std::cerr << "click a window...\n";
        auto picked = pickWindow();
        if (!picked) {
            std::cerr << "no window picked (cancelled, or KWin/D-Bus unavailable)\n";
            return 1;
        }
        pid = picked->pid;
        desktopFile = picked->desktopFile;
    }

    auto info = readProcInfo(pid);
    if (!info) {
        std::cerr << "couldn't read /proc/" << pid << " (no such process, or no permission)\n";
        return 1;
    }

    SourceResult source = detectSource(*info);
    if (source.kind == "Native package") {
        std::string pkg = resolvePackageName(info->exe);
        if (!pkg.empty()) source.detail = pkg;
    }

    PanelExtras extras;
    extras.installDate = getInstallDate(info->exe).value_or("");
    extras.processTree = getProcessTree(pid);
    if (!desktopFile.empty() && isNativeDesktopFileInstalled(desktopFile) && isFlatpakInstalled(desktopFile)) {
        extras.dualInstallNote = "Both a native package and a Flatpak of this app are installed.";
    }

    InspectorPanel panel(*info, source, extras);
    panel.show();

    return app.exec();
}
