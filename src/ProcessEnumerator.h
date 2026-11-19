#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct ProcessInfo {
    int pid = 0;
    double cpuPercent = 0.0;
    std::string name;


    // Cumulative CPU time spent executing this process.
    // macOS reports this in Mach absolute-time ticks.
    std::uint64_t cpuTimeTicks = 0;

    // Whether macOS successfully returned task statistics.
    bool hasTaskInfo = false;

    // Resident memory currently mapped into the process.
    std::uint64_t residentMemoryBytes = 0;
};

class ProcessEnumerator {
public:
    std::vector<ProcessInfo> enumerate() const;
};