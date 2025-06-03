#ifndef METRIC_H
#define METRIC_H

#include <string>
#include <chrono>

struct Metric {
    long long timestamp;
    std::string client_ip;
    std::string hostname;
    double cpu_usage;
    double memory_usage;

    Metric(long long ts, const std::string& ip, const std::string& hn, double cpu, double mem)
        : timestamp(ts), client_ip(ip), hostname(hn), cpu_usage(cpu), memory_usage(mem) {}

    std::string toJson() const {
        return "{ \"timestamp\": " + std::to_string(timestamp) +
               ", \"client_ip\": \"" + client_ip + "\"" +
               ", \"hostname\": \"" + hostname + "\"" +
               ", \"cpu_usage\": " + std::to_string(cpu_usage) +
               ", \"memory_usage\": " + std::to_string(memory_usage) + " }";
    }

    static Metric fromJson(const std::string& json_string) {
        return Metric(0, "0.0.0.0", "unknown", 0.0, 0.0);
    }
};

#endif // METRIC_H
