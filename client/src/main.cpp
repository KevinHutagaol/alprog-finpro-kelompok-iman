#include "PerformanceMonitor.h"


int main() {
    PerformanceMonitor performance_monitor(std::chrono::milliseconds(1000));

    performance_monitor.add_counter("CPU TIME", L"\\Processor(_Total)\\% Processor Time");
}
