#pragma once
#include <optional>
#include <string>
#include <vector>

struct ProcInfo {
    int pid = 0;
    std::string exe;               // empty if unreadable
    std::string cmdline;            // argv joined with spaces, for display
    std::vector<std::string> argv;  // argv, individual entries preserved
    std::string cgroup;             // raw /proc/<pid>/cgroup
    std::string environ;            // raw NUL-separated /proc/<pid>/environ
};

std::optional<ProcInfo> readProcInfo(int pid);
std::optional<std::string> getEnvVar(const std::string& environBlob, const std::string& key);
