//
// Created by kevin on 5/28/2025.
//

#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <pdh.h>
#include <string>
#include <thread>
#include <atomic>

struct PdhCounterData {
    std::string counter_name;
    std::wstring pdh_counter_path;
    HCOUNTER hCounter;
    double counter_value;
    PDH_STATUS pdhStatus;

    PdhCounterData(std::string counter_name, std::wstring pdh_counter_path)
        : counter_name(std::move(counter_name)),
          pdh_counter_path(
              std::move(pdh_counter_path)),
          hCounter(nullptr),
          counter_value(0.0),
          pdhStatus(ERROR_SUCCESS) {
    }
};

class PerformanceMonitor {
public:
    explicit PerformanceMonitor(std::chrono::milliseconds sampling_time);

    void add_counter(const std::string &counter_name, const std::wstring &pdh_counter_path);

    ~PerformanceMonitor();

private:
    std::vector<PdhCounterData> pdh_counters_{};
    std::chrono::milliseconds sampling_time_;
    HQUERY hQuery_;
    std::thread thread_;
    std::atomic<bool> is_monitoring_;
    std::mutex data_mutex_;
};


#endif //PERFORMANCEMONITOR_H
