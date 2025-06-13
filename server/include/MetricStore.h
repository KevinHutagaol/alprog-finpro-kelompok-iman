#ifndef METRIC_STORE_H
#define METRIC_STORE_H

#include "TimeSeriesPoint.h"
#include "ClientData.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>

class MetricStore {
public:
    void addData(const ClientData& data);

    void print() const;

private:
    std::map<std::string, std::vector<TimeSeriesPoint>> time_series_data_;

    mutable std::mutex mutex_;
};

#endif // METRIC_STORE_H
