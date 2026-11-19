//
// Created by Henil Patel  on 2026-11-19.
//


#pragma once

#include "ProcessEnumerator.h"

#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <vector>

class ProcessCpuCollector
{
public:
    ProcessCpuCollector();

    // Calculate CPU percentages using consecutive readings.
    void update(std::vector<ProcessInfo>& processes);

private:
    struct PreviousSample
    {
        std::uint64_t cpuTicks;
        std::chrono::steady_clock::time_point timestamp;
    };

    std::unordered_map<int, PreviousSample> previousSamples_;

    // Conversion factor for Mach absolute-time ticks.
    double nanosecondsPerTick_ = 1.0;
};
