//
// Created by Henil Patel  on 2026-11-19.
//

#include "ProcessCpuCollector.h"

#include <mach/mach_time.h>

ProcessCpuCollector::ProcessCpuCollector()
{
    mach_timebase_info_data_t timebase {};

    // Mach CPU-time counters are not necessarily nanoseconds.
    // Obtain the conversion factor for this Mac.
    if (mach_timebase_info(&timebase) == KERN_SUCCESS &&
        timebase.denom != 0)
    {
        nanosecondsPerTick_ =
            static_cast<double>(timebase.numer) /
            static_cast<double>(timebase.denom);
    }
}

void ProcessCpuCollector::update(
    std::vector<ProcessInfo>& processes)
{
    using Clock = std::chrono::steady_clock;

    const auto now = Clock::now();

    // Build a fresh map so terminated processes are removed.
    std::unordered_map<int, PreviousSample> currentSamples;

    for (auto& process : processes)
    {
        process.cpuPercent = 0.0;

        if (!process.hasTaskInfo)
            continue;

        const auto previous = previousSamples_.find(process.pid);

        if (previous != previousSamples_.end())
        {
            const auto& old = previous->second;

            const double elapsedSeconds =
                std::chrono::duration<double>(
                    now - old.timestamp
                ).count();

            // CPU counters must only increase for the same process.
            if (elapsedSeconds > 0.0 &&
                process.cpuTimeTicks >= old.cpuTicks)
            {
                const auto deltaTicks =
                    process.cpuTimeTicks - old.cpuTicks;

                const double cpuSeconds =
                    static_cast<double>(deltaTicks) *
                    nanosecondsPerTick_ / 1'000'000'000.0;

                // 100% means one logical CPU core fully utilized.
                process.cpuPercent =
                    (cpuSeconds / elapsedSeconds) * 100.0;
            }
        }

        currentSamples[process.pid] = {
            process.cpuTimeTicks,
            now
        };
    }

    previousSamples_ = std::move(currentSamples);
}
