#include "file_info.h"

#include <chrono>
#include <ctime>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * Reads the exe file's last-write time as a stand-in for its install date.
 * Packaging generally doesn't touch a binary after laying it down, so mtime
 * is a reasonable proxy across every source kind (native, Flatpak, AppImage,
 * Snap, manual) without a per-source package-database lookup.
 * @param exePath absolute path to the executable
 * @return formatted "YYYY-MM-DD HH:MM" date, or nullopt if unreadable
 */
std::optional<std::string> getInstallDate(const std::string& exePath) {
    if (exePath.empty()) return std::nullopt;

    std::error_code ec;
    fs::file_time_type ftime = fs::last_write_time(exePath, ec);
    if (ec) return std::nullopt;

    auto sysTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    std::time_t cftime = std::chrono::system_clock::to_time_t(sysTime);

    std::tm tmBuf{};
    localtime_r(&cftime, &tmBuf);

    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tmBuf);
    return std::string(buf);
}
