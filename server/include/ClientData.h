//
// Created by kevin on 13/06/2025.
//

#ifndef CLIENTDATA_H
#define CLIENTDATA_H

#include "MetricDataPoint.h"
#include <string>
#include <vector>
#include <chrono>

struct ClientData {
    std::string clientId;
    std::string clientIp;

    std::chrono::system_clock::time_point timestamp;
    std::vector<MetricDataPoint> metrics;
};

#endif //CLIENTDATA_H
