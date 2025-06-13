#include "MetricStore.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <nlohmann/json.hpp>

std::string format_ts_for_print(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_utc{};
#ifdef _WIN32
    gmtime_s(&tm_utc, &time_t);
#else
    gmtime_r(&time_t, &tm_utc);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

void MetricStore::addData(const ClientData& data) {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto& batch_timestamp = data.timestamp;

    for (const auto& metric_dp : data.metrics) {
        TimeSeriesPoint new_point{batch_timestamp, metric_dp.value};

        time_series_data_[metric_dp.name].push_back(new_point);
    }
}

void MetricStore::print() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (time_series_data_.empty()) {
        std::cout << "  (Store is empty)" << std::endl;
        return;
    }

    for (const auto& pair : time_series_data_) {
        const auto& metric_name = pair.first;
        const auto& points = pair.second;
        std::cout << "  Metric: \"" << metric_name << "\" (" << points.size() << " points)" << std::endl;
        const size_t start_index = (points.size() > 5) ? points.size() - 5 : 0;
        for (size_t i = start_index; i < points.size(); ++i) {
            std::cout << "    - " << format_ts_for_print(points[i].timestamp)
                      << ", Value: " << points[i].value << std::endl;
        }
    }
}

nlohmann::json MetricStore::exportToJson() const {
    // Lock the mutex to ensure a thread-safe read of the data
    std::lock_guard<std::mutex> lock(mutex_);

    // The magic happens here. Because we taught the library how to handle
    // a TimeSeriesPoint, it automatically knows how to handle a
    // std::vector<TimeSeriesPoint> and a std::map<std::string, ...>.
    return nlohmann::json(time_series_data_);
}