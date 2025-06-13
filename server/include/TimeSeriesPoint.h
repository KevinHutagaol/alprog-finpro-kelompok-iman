//
// Created by kevin on 13/06/2025.
//

#ifndef TIMESERIESPOINT_H
#define TIMESERIESPOINT_H
#include <chrono>

struct TimeSeriesPoint {
    std::chrono::system_clock::time_point timestamp;
    double value;
};

#endif //TIMESERIESPOINT_H
