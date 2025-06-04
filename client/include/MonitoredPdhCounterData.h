//
// Created by kevin on 04/06/2025.
//

#ifndef MONITOREDPDHCOUNTERDATA_H
#define MONITOREDPDHCOUNTERDATA_H
#include <pdh.h>
#include <string>

struct MonitoredPdhCounterData {
    std::string counter_name;
    std::wstring pdh_counter_path;
    HCOUNTER hCounter;
    double counter_value;
    PDH_STATUS pdhStatus;
    std::chrono::system_clock::time_point timestamp;

    MonitoredPdhCounterData(std::string counter_name, std::wstring pdh_counter_path)
        : counter_name(std::move(counter_name)),
          pdh_counter_path(
              std::move(pdh_counter_path)),
          hCounter(nullptr),
          counter_value(0.0),
          pdhStatus(ERROR_SUCCESS),
          timestamp(std::chrono::system_clock::now()) {
    }

    friend std::ostream &operator<<(std::ostream &os, const MonitoredPdhCounterData &data);
};


inline std::ostream &operator<<(std::ostream &os, const MonitoredPdhCounterData &data) {
    const std::time_t time_t_stamp = std::chrono::system_clock::to_time_t(data.timestamp);
    std::tm tm_stamp;
    localtime_s(&tm_stamp, &time_t_stamp);

    os << "Timestamp:          " << std::put_time(&tm_stamp, "%Y-%m-%d %H:%M:%S") << "\n"
            << "Counter Name:       " << data.counter_name << "\n"
            << "Last Value:         " << std::fixed << std::setprecision(4) << data.counter_value << "\n";

    os << std::dec;
    return os;
}

#endif //MONITOREDPDHCOUNTERDATA_H
