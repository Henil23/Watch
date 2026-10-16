//
// Created by Henil Patel  on 2026-10-16.
//
#include "MemoryCollector.h"

#include <mach/mach.h>
#include <mach/mach_host.h>
#include <sys/sysctl.h>
#include <cstddef>

MemoryMetrics MemoryCollector::sample() const
{
    MemoryMetrics metrics;

    // Get the total amount of physical RAM installed in the Mac.
    std::uint64_t totalMemory = 0;
    std::size_t size = sizeof(totalMemory);

    if (sysctlbyname(
            "hw.memsize",
            &totalMemory,
            &size,
            nullptr,
            0) == 0)
    {
        metrics.totalBytes = totalMemory;
    }

    vm_statistics64_data_t vmStats {};
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;

    // Ask macOS for current virtual memory statistics.
    const kern_return_t result = host_statistics64(
        mach_host_self(),
        HOST_VM_INFO64,
        reinterpret_cast<host_info64_t>(&vmStats),
        &count
    );

    if (result != KERN_SUCCESS)
    {
        return metrics;
    }

    // macOS reports memory in pages, so we need the system page size
    // to convert page counts into bytes.
    vm_size_t pageSize = 0;
    host_page_size(mach_host_self(), &pageSize);

    metrics.freeBytes =
        static_cast<std::uint64_t>(vmStats.free_count) * pageSize;

    metrics.activeBytes =
        static_cast<std::uint64_t>(vmStats.active_count) * pageSize;

    metrics.inactiveBytes =
        static_cast<std::uint64_t>(vmStats.inactive_count) * pageSize;

    metrics.wiredBytes =
        static_cast<std::uint64_t>(vmStats.wire_count) * pageSize;

    metrics.compressedBytes =
        static_cast<std::uint64_t>(vmStats.compressor_page_count) * pageSize;
    // Approximate actively occupied memory.
    // Inactive/cached memory is excluded because macOS can reclaim it when needed.
    metrics.usedBytes =
        metrics.activeBytes +
        metrics.wiredBytes +
        metrics.compressedBytes;

    return metrics;
}