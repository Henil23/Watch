
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

#include "CpuCollector.h"
#include "MemoryCollector.h"
#include "ProcessEnumerator.h"
#include "ProcessCpuCollector.h"

int main()
{
    CpuCollector cpuCollector;
    MemoryCollector memoryCollector;
    ProcessEnumerator processEnumerator;
    ProcessCpuCollector processCpuCollector;

    constexpr double GiB = 1024.0 * 1024.0 * 1024.0;
    constexpr double MiB = 1024.0 * 1024.0;

    std::cout << "Watch started.\n";

    while (true)
    {
        // Collect system-wide CPU and RAM information.
        const auto cpu = cpuCollector.sample();
        const auto memory = memoryCollector.sample();

        // Discover running processes and update their CPU usage.
        auto processes = processEnumerator.enumerate();
        processCpuCollector.update(processes);

        // Rank processes by CPU utilization.
        std::sort(
            processes.begin(),
            processes.end(),
            [](const ProcessInfo& a, const ProcessInfo& b)
            {
                return a.cpuPercent > b.cpuPercent;
            }
        );

        std::cout << "\n========== WATCH ==========\n";

        std::cout << std::fixed << std::setprecision(2);

        std::cout << "System CPU: " << cpu.totalPercent << "%\n";
        std::cout << "Memory: "
                  << memory.usedBytes / GiB << " / "
                  << memory.totalBytes / GiB << " GiB\n";

        std::cout << "Processes: " << processes.size() << "\n\n";

        std::cout << std::left
                  << std::setw(8)  << "PID"
                  << std::setw(32) << "PROCESS"
                  << std::setw(12) << "CPU (%)"
                  << "RAM (MiB)\n";

        std::cout << std::string(65, '-') << '\n';

        const std::size_t count =
            std::min(processes.size(), std::size_t{10});

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto& process = processes[i];

            // Truncate long names to keep columns aligned.
            const std::string name = process.name.substr(0, 30);

            std::cout << std::left
                      << std::setw(8) << process.pid
                      << std::setw(32) << name
                      << std::setw(12) << process.cpuPercent
                      << process.residentMemoryBytes / MiB
                      << '\n';
        }

        // Wait approximately one second before collecting again.
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
