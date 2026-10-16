//
// Created by Henil Patel  on 2026-10-16.
//

#ifndef WATCH_MEMORYCOLLECTOR_H
#define WATCH_MEMORYCOLLECTOR_H

#endif //WATCH_MEMORYCOLLECTOR_H
#pragma once

#include <cstdint> // helps in defining fixed width integer bytes such as uint

// Represents the main memory stats we care about for our app.
struct MemoryMetrics
{
    // the diff between int_t and standard int is that int_t confirms the bit size and it is platform independent
    // whereas using standard int cause varying bit sizes depending on the hardware they are installed.
    // for mem optimization we use int_t provided yb cstdint.


    uint64_t totalBytes =0;
    uint64_t usedBytes =0;
    uint64_t freeBytes =0;

    uint64_t activeBytes =0;
    uint64_t inactiveBytes =0;
    uint64_t wiredBytes=0;
    uint64_t compressedBytes=0;
};

class MemoryCollector {
public:
    // Read the current memory state from macOS.
    MemoryMetrics sample() const;
};