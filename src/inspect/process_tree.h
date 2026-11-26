#pragma once
#include <string>
#include <vector>

struct ProcessTreeEntry {
    int pid = 0;
    std::string name;
};

std::vector<ProcessTreeEntry> getProcessTree(int pid);
