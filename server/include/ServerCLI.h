#ifndef SERVERCLI_H
#define SERVERCLI_H

#include <vector>
#include <string>
#include <atomic>
#include <thread>

#include "Metric.h"
#include "MetricScore.h"

class CLI {
public:
    CLI();
    ~CLI();

    void run();

private:
    void processInputLine(const std::string& line);
    void displayPrompt() const;
    void printHelp() const;

    void handleStartMonitoring(const std::vector<std::string>& args);
    void handleStopMonitoring();

    void handlePrintSortedByCpu(const std::vector<std::string>& args);

    void handlePrintSortedByMemory(const std::vector<std::string>& args);

    void handleSaveMetricsBinary(const std::vector<std::string>& args);

    void handleExportMetricsJson(const std::vector<std::string>& args);

    void handlePrintAllClients() const;

    void handlePrintAllMetrics() const;

    void periodicMonitoringTask();

    void printMetricsTable(const std::vector<Metric>& metricsToPrint) const;

    void printSingleMetric(const Metric& metric) const;

    MetricStore& store;

    std::thread monitorThread;
    std::atomic<bool> monitoringActive;
    std::atomic<int> monitorIntervalSeconds;

    mutable std::mutex outputMutex;
};



#endif //SERVERCLI_H

