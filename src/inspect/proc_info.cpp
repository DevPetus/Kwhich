#include "proc_info.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace fs = std::filesystem;

namespace {

/**
 * Reads an entire file into a string.
 * @param path file to read
 * @return file contents, or empty string if it can't be opened
 */
std::string readWholeFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

/**
 * Splits /proc/<pid>/cmdline's raw NUL-separated bytes back into individual
 * argv entries, preserving any spaces within a single argument.
 * @param raw raw contents of /proc/<pid>/cmdline
 * @return argv entries in order, or empty if raw was empty/unreadable
 */
std::vector<std::string> splitCmdlineArgv(const std::string& raw) {
    std::vector<std::string> argv;
    size_t start = 0;
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\0') {
            if (i > start) argv.push_back(raw.substr(start, i - start));
            start = i + 1;
        }
    }
    if (start < raw.size()) argv.push_back(raw.substr(start));
    return argv;
}

}  // namespace

/**
 * Reads the subset of /proc/<pid>/ that the source-detection rules need:
 * the exe symlink target, cmdline, cgroup and environ. Fields that can't be
 * read (permission denied, root-owned process) are left empty on the
 * returned ProcInfo rather than failing the whole read.
 * @param pid process id to inspect
 * @return populated ProcInfo, or nullopt if the process doesn't exist
 */
std::optional<ProcInfo> readProcInfo(int pid) {
    fs::path base = "/proc/" + std::to_string(pid);
    std::error_code ec;
    if (!fs::exists(base, ec)) return std::nullopt;

    ProcInfo info;
    info.pid = pid;

    std::error_code exeEc;
    fs::path exeLink = fs::read_symlink(base / "exe", exeEc);
    if (!exeEc) info.exe = exeLink.string();

    info.argv = splitCmdlineArgv(readWholeFile(base / "cmdline"));
    for (size_t i = 0; i < info.argv.size(); ++i) {
        if (i > 0) info.cmdline += ' ';
        info.cmdline += info.argv[i];
    }

    info.cgroup = readWholeFile(base / "cgroup");
    info.environ = readWholeFile(base / "environ");

    return info;
}

/**
 * Looks up one KEY=value entry inside a NUL-separated /proc/<pid>/environ blob.
 * @param environBlob raw environ contents, as read by readProcInfo
 * @param key environment variable name to find
 * @return the value for key, or nullopt if it isn't present
 */
std::optional<std::string> getEnvVar(const std::string& environBlob, const std::string& key) {
    const std::string prefix = key + "=";
    size_t pos = 0;
    while (pos < environBlob.size()) {
        size_t end = environBlob.find('\0', pos);
        if (end == std::string::npos) end = environBlob.size();
        std::string entry = environBlob.substr(pos, end - pos);
        if (entry.rfind(prefix, 0) == 0) return entry.substr(prefix.size());
        pos = end + 1;
    }
    return std::nullopt;
}