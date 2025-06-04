//
// Created by kevin on 5/28/2025.
//

#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H

#include <chrono>
#include <pdh.h>
#include <string>
#include <thread>
#include <functional>
#include <atomic>
#include <mutex>

#include "MonitoredPdhCounterData.h"


class PerformanceMonitor {
public:
    explicit PerformanceMonitor(std::chrono::milliseconds sampling_time);

    bool add_counter(const std::string &counter_name, const std::wstring &pdh_counter_path);

    bool initialize();

    void uninitialize();

    void start_monitoring();

    void stop_monitoring();

    void set_callback(const std::function<void(const std::vector<MonitoredPdhCounterData> &)> &callback_fn_);

    [[nodiscard]] std::vector<MonitoredPdhCounterData> get_current_snapshot() const;

    ~PerformanceMonitor();

private:
    void monitoring_loop();

    bool collect_pdh_counter_data();

    std::function<void(const std::vector<MonitoredPdhCounterData> &)> callback_fn_;

    std::vector<MonitoredPdhCounterData> pdh_counters_{};
    std::chrono::milliseconds sampling_time_;
    HQUERY hQuery_;
    bool is_initialized_;

    std::thread monitor_thread_;
    std::atomic<bool> is_monitoring_;
    mutable std::mutex pdh_counter_mutex_;
};


#endif //PERFORMANCEMONITOR_H
