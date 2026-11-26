#include "shell.h"

#include <array>
#include <cstdio>
#include <memory>

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
