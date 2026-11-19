#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct ProcessInfo {
    int pid = 0;
    std::string name;

    // Resident memory currently mapped into the process.
    std::uint64_t residentMemoryBytes = 0;
};

class ProcessEnumerator {
public:
    std::vector<ProcessInfo> enumerate() const;
};