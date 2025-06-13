#ifndef METRICDATAPOINT_H
#define METRICDATAPOINT_H

#include <string>
#include <nlohmann/json.hpp>

struct MetricDataPoint {
    std::string name;
    double value;
};

inline void from_json(const nlohmann::json& j, MetricDataPoint& p) {
    j.at("name").get_to(p.name);
    j.at("value").get_to(p.value);
}



#endif //METRICDATAPOINT_H
