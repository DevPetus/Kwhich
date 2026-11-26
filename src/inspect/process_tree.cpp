#include "process_tree.h"

#include <fstream>
#include <sstream>

namespace {

struct StatusFields {
    std::string name;
    int ppid = 0;
    bool ok = false;
};

/**
 * Reads the Name and PPid lines out of /proc/<pid>/status.
 * @param pid process to read
 * @return parsed fields; ok is false if the file was unreadable or was
 *         missing either key (process gone, or no permission)
 */
StatusFields readStatus(int pid) {
    std::ifstream in("/proc/" + std::to_string(pid) + "/status");
    StatusFields fields;
    if (!in) return fields;

    bool haveName = false;
    bool havePpid = false;
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("Name:", 0) == 0) {
            std::istringstream ss(line.substr(5));
            ss >> fields.name;
            haveName = true;
        } else if (line.rfind("PPid:", 0) == 0) {
            fields.ppid = std::stoi(line.substr(5));
            havePpid = true;
        }
        if (haveName && havePpid) break;
    }
    fields.ok = haveName && havePpid;
    return fields;
}

}  // namespace

/**
 * Walks /proc/<pid>/status's PPid chain from pid up toward pid 1, collecting
 * each ancestor's name. Useful for spotting a launcher/wrapper chain (e.g. a
 * Steam game run through its runtime script). Capped to a sane depth so a
 * corrupt or looping chain can't hang the caller.
 * @param pid pid to start from
 * @return entries from pid itself up through its ancestors, closest first
 */
std::vector<ProcessTreeEntry> getProcessTree(int pid) {
    std::vector<ProcessTreeEntry> chain;
    constexpr int maxDepth = 16;

    int current = pid;
    for (int depth = 0; depth < maxDepth && current > 0; ++depth) {
        StatusFields fields = readStatus(current);
        if (!fields.ok) break;
        chain.push_back({current, fields.name});
        if (fields.ppid <= 0 || fields.ppid == current) break;
        current = fields.ppid;
    }
    return chain;
}
