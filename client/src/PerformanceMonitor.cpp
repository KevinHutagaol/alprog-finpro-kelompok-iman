#include "PerformanceMonitor.h"

#include <iostream>


PerformanceMonitor::PerformanceMonitor(const std::chrono::milliseconds sampling_time)
    : sampling_time_(sampling_time),
      hQuery_(nullptr),
      is_initialized_(false),
      is_monitoring_(false) {
}

bool PerformanceMonitor::add_counter(const std::string &counter_name, const std::wstring &pdh_counter_path) {
    if (this->is_initialized_) {
        std::cerr << "PerformanceMonitor is already initialized, unable to add new counter" << std::endl;
        return false;
    }
    if (this->is_monitoring_.load() || this->monitor_thread_.joinable()) {
        std::cerr << "PerformanceMonitor is Monitoring, unable to add new counter" << std::endl;
        return false;
    }
    this->pdh_counters_.emplace_back(counter_name, pdh_counter_path);
    return true;
}

bool PerformanceMonitor::initialize() {
    if (this->is_initialized_) {
        std::cerr << "PerformanceMonitor is already initialized" << std::endl;
        return true;
    }

    auto pdh_status = PdhOpenQueryW(nullptr, 0, &hQuery_);
    if (pdh_status != ERROR_SUCCESS) {
        std::cout << "PdhOpenQuery failed with 0x" << std::hex << pdh_status << std::endl;
        this->hQuery_ = nullptr;
        return false;
    }

    if (this->pdh_counters_.empty()) {
        std::cout << "Warn: PerformanceMonitor does not have any counters" << std::endl;
    }

    bool added_counter = false;
    for (auto &counter: this->pdh_counters_) {
        pdh_status = PdhAddCounterW(this->hQuery_, counter.pdh_counter_path.c_str(), 0, &counter.hCounter);
        if (pdh_status != ERROR_SUCCESS) {
            std::cerr << "PdhAddCounter failed with 0x" << std::hex << pdh_status << " for: " << counter.counter_name <<
                    std::endl;
            counter.hCounter = nullptr;
        } else {
            added_counter = true;
        }
    }

    pdh_status = PdhCollectQueryData(this->hQuery_);
    if (pdh_status != ERROR_SUCCESS) {
        std::cerr << "Initial PdhCollectQueryData failed with 0x" << std::hex << pdh_status << std::endl;
        this->uninitialize();
        return false;
    }

    this->is_initialized_ = true;
    std::cout << "PerformanceMonitor initialized successfully." << std::endl;
    if (!this->pdh_counters_.empty() && !added_counter) {
        std::cerr << "Warning: PerformanceMonitor initialized, but no defined counters were added successfully." <<
                std::endl;
    }
    return true;
}

void PerformanceMonitor::uninitialize() {
    if (this->is_monitoring_.load() || this->monitor_thread_.joinable()) {
        std::cerr << "PerformanceMonitor is Monitoring, unable unitialize counters" << std::endl;
        return;
    }
    if (this->hQuery_ != nullptr) {
        PdhCloseQuery(this->hQuery_);
        this->hQuery_ = nullptr;
    }

    for (auto &counter: this->pdh_counters_) {
        counter.hCounter = nullptr;
    }

    is_initialized_ = false;
}

bool PerformanceMonitor::collect_pdh_counter_data() {
    if (!this->is_initialized_) {
        std::cerr << "PerformanceMonitor is not yet initialized" << std::endl;
        return false;
    }

    auto pdh_status = PdhCollectQueryData(this->hQuery_);
    auto current_sample_time = std::chrono::system_clock::now();

    if (pdh_status != ERROR_SUCCESS) {
        std::cerr << "PdhCollectQueryData failed with 0x" << std::hex << pdh_status << std::endl;
        return false;
    }

    for (auto &counter: this->pdh_counters_) {
        counter.timestamp = current_sample_time;
        PDH_FMT_COUNTERVALUE DisplayValue;
        DWORD CounterType;

        if (counter.hCounter == nullptr) {
            continue;
        }

        counter.pdhStatus = PdhGetFormattedCounterValue(counter.hCounter,
                                                        PDH_FMT_DOUBLE,
                                                        &CounterType,
                                                        &DisplayValue);

        if (counter.pdhStatus != ERROR_SUCCESS) {
            std::cerr << "PdhGetFormattedCounterValue failed with 0x" << std::hex << counter.pdhStatus << " for: "
                    << counter.counter_name << std::endl;
            continue;
        }
        counter.counter_value = DisplayValue.doubleValue;
    }

    return true;
}

// multithreading stuff

void PerformanceMonitor::set_callback(
    const std::function<void(const std::vector<MonitoredPdhCounterData> &)> &callback_fn_) {
    if (this->is_monitoring_.load() || this->monitor_thread_.joinable()) {
        std::cerr << "PerformanceMonitor is Monitoring, unable to set callback function" << std::endl;
        return;
    }
    this->callback_fn_ = callback_fn_;
}

std::vector<MonitoredPdhCounterData> PerformanceMonitor::get_current_snapshot() const {
    if (!this->is_initialized_) {
        std::cerr <<
                "Performance Monitor is uninitialized, unable to get snapshot" << std::endl;
        return {};
    }

    std::lock_guard lock(this->pdh_counter_mutex_);
    return this->pdh_counters_;
}

void PerformanceMonitor::start_monitoring() {
    if (!this->is_initialized_) {
        std::cerr << "PerformanceMonitor not initialized. Call initialize() first." << std::endl;
        return;
    }
    if (this->is_monitoring_.load() || this->monitor_thread_.joinable()) {
        std::cerr << "Monitoring thread is already running." << std::endl;
        return;
    }
    if (this->callback_fn_ == nullptr) {
        std::cerr << "PerformanceMonitor callback function is not set." << std::endl;
        return;
    }

    this->is_monitoring_.store(true);
    this->monitor_thread_ = std::thread(&PerformanceMonitor::monitoring_loop, this);
}

void PerformanceMonitor::stop_monitoring() {
    this->is_monitoring_.store(false);
    if (this->monitor_thread_.joinable()) {
        this->monitor_thread_.join();
    }
}

void PerformanceMonitor::monitoring_loop() {
    std::cout << "Started monitoring PerformanceMonitor" << std::endl;

    while (this->is_monitoring_.load() == true) {
        std::this_thread::sleep_for(this->sampling_time_);

        if (this->is_monitoring_.load() == false) { break; }

        if (!collect_pdh_counter_data()) {
            std::cerr << "collect_pdh_counter_data while monitoring" << std::endl;
        }


        if (this->callback_fn_ != nullptr) {
            std::vector<MonitoredPdhCounterData> snapshot_pdh_counters; {
                std::lock_guard lock(this->pdh_counter_mutex_);
                snapshot_pdh_counters = this->pdh_counters_;
            }

            this->callback_fn_(snapshot_pdh_counters);
        }
    }
}

PerformanceMonitor::~PerformanceMonitor() {
    stop_monitoring();
    uninitialize();
}
