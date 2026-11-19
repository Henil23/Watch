
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

#include "CpuCollector.h"
#include "MemoryCollector.h"
#include "ProcessEnumerator.h"

int main()
{
    // Initialize the macOS system resource collectors.
    CpuCollector cpuCollector;
    MemoryCollector memoryCollector;
    ProcessEnumerator processEnumerator;

    std::cout << "Watch started.\n\n";

    // Continuously monitor system resources every second.
    while (true)
    {
        // Collect system-wide CPU and memory statistics.
        const CpuMetrics cpu = cpuCollector.sample();
        const MemoryMetrics memory = memoryCollector.sample();

        // Convert memory values from bytes to GiB.
        constexpr double bytesPerGiB = 1024.0 * 1024.0 * 1024.0;

        const double usedGB =
            static_cast<double>(memory.usedBytes) / bytesPerGiB;

        const double totalGB =
            static_cast<double>(memory.totalBytes) / bytesPerGiB;

        // Display system-wide resource usage.
        std::cout << "\n========== WATCH ==========\n";

        std::cout << std::fixed << std::setprecision(2);

        std::cout << "CPU Usage: " << cpu.totalPercent << "%\n"
                  << "Memory: " << usedGB << " / "
                  << totalGB << " GiB\n";

        // Retrieve all running processes from macOS.
        auto processes = processEnumerator.enumerate();

        std::cout << "Running Processes: "
                  << processes.size() << "\n";

        // Sort processes in descending order of resident memory usage.
        std::sort(
            processes.begin(),
            processes.end(),
            [](const ProcessInfo& a, const ProcessInfo& b)
            {
                return a.residentMemoryBytes > b.residentMemoryBytes;
            }
        );

        // Display the 10 processes consuming the most resident memory.
        std::cout << "\nTop 10 Processes by Memory\n";
        std::cout << "---------------------------------------------\n";

        std::cout << std::left
                  << std::setw(10) << "PID"
                  << std::setw(25) << "PROCESS"
                  << "MEMORY (MiB)\n";

        std::cout << "---------------------------------------------\n";

        const std::size_t displayCount =
            std::min(processes.size(), std::size_t{10});

        constexpr double bytesPerMiB = 1024.0 * 1024.0;

        for (std::size_t i = 0; i < displayCount; ++i)
        {
            const ProcessInfo& process = processes[i];

            const double memoryMB =
                static_cast<double>(process.residentMemoryBytes) /
                bytesPerMiB;

            std::cout << std::left
                      << std::setw(10) << process.pid
                      << std::setw(25) << process.name
                      << std::fixed << std::setprecision(1)
                      << memoryMB << '\n';
        }

        std::cout << "=============================================\n";

        // Wait one second before the next sampling cycle.
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
