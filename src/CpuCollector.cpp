//
// Created by Henil Patel  on 2026-10-14.
//
#include "CpuCollector.h"
#include <mach/mach.h>
#include <mach/host_info.h>
#include <mach/mach_host.h>

CpuMetrics CpuCollector::sample()
{
    host_cpu_load_info_data_t cpuInfo {};
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;

    // Ask macOS for cumulative CPU tick counts since boot.
    kern_return_t result = host_statistics(
        mach_host_self(),
        HOST_CPU_LOAD_INFO,
        reinterpret_cast<host_info_t>(&cpuInfo),
        &count
    );

    // If macOS fails to return CPU information,
    // return an empty/default metrics object for now.
    if (result != KERN_SUCCESS) {
        return {};
    }

    const auto user =
        static_cast<unsigned long long>(cpuInfo.cpu_ticks[CPU_STATE_USER]);

    const auto system =
        static_cast<unsigned long long>(cpuInfo.cpu_ticks[CPU_STATE_SYSTEM]);

    const auto idle =
        static_cast<unsigned long long>(cpuInfo.cpu_ticks[CPU_STATE_IDLE]);

    const auto nice =
        static_cast<unsigned long long>(cpuInfo.cpu_ticks[CPU_STATE_NICE]);

    // The first reading cannot produce a percentage because CPU usage
    // is calculated from the difference between two samples.
    if (firstSample_) {
        prevUser_ = user;
        prevSystem_ = system;
        prevIdle_ = idle;
        prevNice_ = nice;

        firstSample_ = false;

        return {};
    }

    // Calculate how many CPU ticks occurred since the previous sample.
    const auto userDelta = user - prevUser_;
    const auto systemDelta = system - prevSystem_;
    const auto idleDelta = idle - prevIdle_;
    const auto niceDelta = nice - prevNice_;

    const auto totalDelta =
        userDelta + systemDelta + idleDelta + niceDelta;

    const auto activeDelta =
        userDelta + systemDelta + niceDelta;

    // Store the current values so the next sample can compare against them.
    prevUser_ = user;
    prevSystem_ = system;
    prevIdle_ = idle;
    prevNice_ = nice;

    if (totalDelta == 0) {
        return {};
    }

    CpuMetrics metrics;

    metrics.totalPercent =
        (static_cast<double>(activeDelta) /
         static_cast<double>(totalDelta)) * 100.0;

    return metrics;
}