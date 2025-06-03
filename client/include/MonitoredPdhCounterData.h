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

    MonitoredPdhCounterData(std::string counter_name, std::wstring pdh_counter_path)
        : counter_name(std::move(counter_name)),
          pdh_counter_path(
              std::move(pdh_counter_path)),
          hCounter(nullptr),
          counter_value(0.0),
          pdhStatus(ERROR_SUCCESS) {
    }
};

#endif //MONITOREDPDHCOUNTERDATA_H
