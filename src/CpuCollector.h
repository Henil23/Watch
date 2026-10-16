//
// Created by Henil Patel  on 2026-10-14.
//

#ifndef WATCH_CPUCOLLECTOR_H
#define WATCH_CPUCOLLECTOR_H

#endif //WATCH_CPUCOLLECTOR_H
#pragma once
struct CpuMetrics {
    double totalPercent = 0.0;
};

class CpuCollector {
    public:
    CpuMetrics sample();
private:
    unsigned long long prevUser_ = 0;
    unsigned long long prevSystem_ = 0;
    unsigned long long prevIdle_ = 0;
    unsigned long long prevNice_ = 0;

    bool firstSample_ = true;

};