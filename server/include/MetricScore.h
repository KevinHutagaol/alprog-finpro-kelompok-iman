#ifndef METRIC_STORE_H
#define METRIC_STORE_H

#include "Metric.h"
#include <vector>
#include <string>
#include <mutex>
#include <fstream>
#include <algorithm>

class MetricStore {
public:
    static MetricStore& getInstance() {
        static MetricStore instance;
        return instance;
    }

    void saveMetric(const Metric& metric);
    void saveAllMetricsToBinaryFile(const std::string& filename) const;
    void loadAllMetricsFromBinaryFile(const std::string& filename);
    void exportMetricsToJsonFile(const std::string& filename, long long startTime, long long endTime) const;

    std::vector<Metric> getMetricsByHostname(const std::string& hostname) const;
    std::vector<Metric> getMetricsByTimeRange(long long startTime, long long endTime) const;
    std::vector<Metric> getSortedMetricsByCpuUsage() const;
    std::vector<Metric> getSortedMetricsByMemoryUsage() const;

    std::vector<Metric> getAllMetrics() const;

private:
    MetricStore() = default;
    MetricStore(const MetricStore&) = delete;
    MetricStore& operator=(const MetricStore&) = delete;

    std::vector<Metric> metrics;
    mutable std::mutex metricsMutex;
};

#endif // METRIC_STORE_H
