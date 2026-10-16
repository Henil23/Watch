#include <chrono>
#include <iostream>
#include <thread>

#include "CpuCollector.h"
#include "MemoryCollector.h"

int main()
{
    CpuCollector cpuCollector;
    MemoryCollector memoryCollector;

    std::cout << "Watch started.\n\n";

    while (true)
    {
        const CpuMetrics cpu = cpuCollector.sample();
        const MemoryMetrics memory = memoryCollector.sample();

        const double usedGB =
            static_cast<double>(memory.usedBytes) / (1024.0 * 1024.0 * 1024.0);

        const double totalGB =
            static_cast<double>(memory.totalBytes) / (1024.0 * 1024.0 * 1024.0);

        std::cout << "CPU: "
                  << cpu.totalPercent
                  << "% | Memory: "
                  << usedGB
                  << " / "
                  << totalGB
                  << " GB\n";

        // Take a new system snapshot once every second.
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}