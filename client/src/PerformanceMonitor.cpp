#include "PerformanceMonitor.h"

#include <iostream>


PerformanceMonitor::PerformanceMonitor(const std::chrono::milliseconds sampling_time): sampling_time_(sampling_time) {
}

void PerformanceMonitor::add_counter(const std::string &counter_name, const std::wstring &pdh_counter_path) {
}

PerformanceMonitor::~PerformanceMonitor() {
}
