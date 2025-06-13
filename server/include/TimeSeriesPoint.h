//
// Created by kevin on 13/06/2025.
//

#ifndef TIMESERIESPOINT_H
#define TIMESERIESPOINT_H
#include <chrono>
#include <nlohmann/json.hpp>

struct TimeSeriesPoint {
    std::chrono::system_clock::time_point timestamp;
    double value;
};

inline void to_json(nlohmann::json &j, const TimeSeriesPoint &p) {
    auto time_t_value = std::chrono::system_clock::to_time_t(p.timestamp);

    std::tm tm_utc{};
#ifdef _WIN32
    gmtime_s(&tm_utc, &time_t_value);
#else
    gmtime_r(&time_t_value, &tm_utc);
#endif

    std::stringstream ss;
    ss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");


    j = nlohmann::json{
        {"timestamp", ss.str()},
        {"value", p.value}
    };
}

#endif //TIMESERIESPOINT_H
