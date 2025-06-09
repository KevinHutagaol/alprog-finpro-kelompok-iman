//#include "MetricStore.h"
//#include <iostream>
//#include <algorithm>
//#include "json.hpp" // Ini diasumsikan kamu udah pindahin json.hpp ke folder ini
//
//void MetricStore::saveMetric(const Metric& metric) {
//    std::lock_guard<std::mutex> lock(metricsMutex);
//    metrics.push_back(metric);
//    std::cout << "[STORE] Metric saved: IP=" << metric.client_ip << ", CPU=" << metric.cpu_usage << std::endl;
//}
//
//void MetricStore::saveAllMetricsToBinaryFile(const std::string& filename) const {
//    std::lock_guard<std::mutex> lock(metricsMutex);
//    std::ofstream ofs(filename, std::ios::binary);
//    if (!ofs.is_open()) {
//        std::cerr << "[STORE ERROR] Could not open binary file for writing: " << filename << std::endl;
//        return;
//    }
//
//    size_t count = metrics.size();
//    ofs.write(reinterpret_cast<const char*>(&count), sizeof(count));
//
//    for (const auto& metric : metrics) {
//        ofs.write(reinterpret_cast<const char*>(&metric.timestamp), sizeof(metric.timestamp));
//
//        size_t ip_len = metric.client_ip.length();
//        ofs.write(reinterpret_cast<const char*>(&ip_len), sizeof(ip_len));
//        ofs.write(metric.client_ip.data(), ip_len);
//
//        size_t hn_len = metric.hostname.length();
//        ofs.write(reinterpret_cast<const char*>(&hn_len), sizeof(hn_len));
//        ofs.write(metric.hostname.data(), hn_len);
//
//        ofs.write(reinterpret_cast<const char*>(&metric.cpu_usage), sizeof(metric.cpu_usage));
//        ofs.write(reinterpret_cast<const char*>(&metric.memory_usage), sizeof(metric.memory_usage));
//    }
//    ofs.close();
//    std::cout << "[STORE] All metrics saved to binary file: " << filename << std::endl;
//}
//
//void MetricStore::loadAllMetricsFromBinaryFile(const std::string& filename) {
//    std::lock_guard<std::mutex> lock(metricsMutex);
//    std::ifstream ifs(filename, std::ios::binary);
//    if (!ifs.is_open()) {
//        std::cerr << "[STORE ERROR] Could not open binary file for reading: " << filename << std::endl;
//        return;
//    }
//
//    metrics.clear();
//
//    size_t count;
//    ifs.read(reinterpret_cast<char*>(&count), sizeof(count));
//
//    for (size_t i = 0; i < count; ++i) {
//        long long timestamp;
//        std::string client_ip, hostname;
//        double cpu_usage, memory_usage;
//
//        ifs.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
//
//        size_t ip_len, hn_len;
//        ifs.read(reinterpret_cast<char*>(&ip_len), sizeof(ip_len));
//        client_ip.resize(ip_len);
//        ifs.read(&client_ip[0], ip_len);
//
//        ifs.read(reinterpret_cast<char*>(&hn_len), sizeof(hn_len));
//        hostname.resize(hn_len);
//        ifs.read(&hostname[0], hn_len);
//
//        ifs.read(reinterpret_cast<char*>(&cpu_usage), sizeof(cpu_usage));
//        ifs.read(reinterpret_cast<char*>(&memory_usage), sizeof(memory_usage));
//
//        metrics.emplace_back(timestamp, client_ip, hostname, cpu_usage, memory_usage);
//    }
//    ifs.close();
//    std::cout << "[STORE] Loaded " << metrics.size() << " metrics from binary file: " << filename << std::endl;
//}
//
//void MetricStore::exportMetricsToJsonFile(const std::string& filename, long long startTime, long long endTime) const {
//    std::lock_guard<std::mutex> lock(metricsMutex);
//    std::ofstream ofs(filename);
//    if (!ofs.is_open()) {
//        std::cerr << "[STORE ERROR] Could not open JSON file for writing: " << filename << std::endl;
//        return;
//    }
//
//    nlohmann::json j_array = nlohmann::json::array();
//    for (const auto& metric : metrics) {
//        if (metric.timestamp >= startTime && metric.timestamp <= endTime) {
//            nlohmann::json j_metric;
//            j_metric["timestamp"] = metric.timestamp;
//            j_metric["client_ip"] = metric.client_ip;
//            j_metric["hostname"] = metric.hostname;
//            j_metric["cpu_usage"] = metric.cpu_usage;
//            j_metric["memory_usage"] = metric.memory_usage;
//            j_array.push_back(j_metric);
//        }
//    }
//
//    ofs << j_array.dump(4);
//    ofs.close();
//    std::cout << "[STORE] Exported metrics to JSON file: " << filename << std::endl;
//}
//
//
//std::vector<Metric> MetricStore::getMetricsByHostname(const std::string& hostname) const {
//    std::lock_guard<std::mutex> lock(metricsMutex);
//    std::vector<Metric> result;
//    for (const auto& metric : metrics) {
//        if (metric.hostname == hostname) {
//            result.push_back(metric);
//        }
//    }
//    return result;
//}
//
//std::vector<Metric> MetricStore::getMetricsByTimeRange(long long startTime, long long endTime) const {
//    std::lock_guard<std::mutex> lock(metricsMutex);
//    std::vector<Metric> result;
//    for (const auto& metric : metrics) {
//        if (metric.timestamp >= startTime && metric.timestamp <= endTime) {
//            result.push_back(metric);
//        }
//    }
//    std::sort(result.begin(), result.end(), [](const Metric& a, const Metric& b) {
//        return a.timestamp < b.timestamp;
//    });
//    return result;
//}
//
//std::vector<Metric> MetricStore::getSortedMetricsByCpuUsage() const {
//    std::lock_guard<std::mutex> lock(metricsMutex);
//    std::vector<Metric> sorted_metrics = metrics;
//    std::sort(sorted_metrics.begin(), sorted_metrics.end(), [](const Metric& a, const Metric& b) {
//        return a.cpu_usage > b.cpu_usage;
//    });
//    return sorted_metrics;
//}
//
//std::vector<Metric> MetricStore::getSortedMetricsByMemoryUsage() const {
//    std::lock_guard<std::mutex> lock(metricsMutex);
//    std::vector<Metric> sorted_metrics = metrics;
//    std::sort(sorted_metrics.begin(), sorted_metrics.end(), [](const Metric& a, const Metric& b) {
//        return a.memory_usage > b.memory_usage;
//    });
//    return sorted_metrics;
//}
//
//std::vector<Metric> MetricStore::getAllMetrics() const {
//    std::lock_guard<std::mutex> lock(metricsMutex);
//    return metrics;
//}
