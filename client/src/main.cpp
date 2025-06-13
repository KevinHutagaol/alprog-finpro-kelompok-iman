#include "WSClient.h"
#include "PerformanceMonitor.h"

#include <nlohmann/json.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <functional>
#include <atomic>
#include <csignal>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

std::string format_timestamp_iso8601(const std::chrono::system_clock::time_point &tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_utc;
    gmtime_s(&tm_utc, &time_t);
    std::stringstream ss;
    ss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string format_data_to_json(const std::string &client_id, const std::vector<MonitoredPdhCounterData> &data_points) {
    auto timestamp = std::chrono::system_clock::now();
    for (const auto &dp: data_points) {
        if (dp.hCounter != nullptr && dp.pdhStatus == ERROR_SUCCESS) {
            timestamp = dp.timestamp;
            break;
        }
    }

    json counters_array = json::array();
    for (const auto &dp: data_points) {
        if (dp.hCounter == nullptr || dp.pdhStatus != ERROR_SUCCESS) continue;
        counters_array.push_back({
            {"name", dp.counter_name},
            {"value", dp.counter_value}
        });
    }

    json j = {
        {"clientId", client_id},
        {"timestamp", format_timestamp_iso8601(timestamp)},
        {"counters", counters_array}
    };

    return j.dump();
}

std::string get_user_input(const std::string &prompt, const std::string &default_value = "") {
    std::cout << prompt;
    if (!default_value.empty()) {
        std::cout << " [" << default_value << "]";
    }
    std::cout << ": ";

    std::string input;
    std::getline(std::cin, input);

    if (input.empty()) {
        return default_value;
    }
    return input;
}

int main(int argc, char *argv[]) {
    std::cout << "--- WebSocket Performance Monitor Client ---\n";
    std::string host = get_user_input("Enter server host (IP address)", "127.0.0.1");
    std::string port = get_user_input("Enter server port", "6969");
    std::string client_id = get_user_input("Enter a unique Client ID");

    if (client_id.empty()) {
        std::cerr << "Error: Client ID cannot be empty. Exiting." << std::endl;
        return 1;
    }

    std::cout << "\nConfiguration set:" << std::endl;
    std::cout << "  - Client ID:   " << client_id << std::endl;
    std::cout << "  - Server:      " << host << ":" << port << std::endl;
    std::cout << "-------------------------------------------\n" << std::endl;

    boost::asio::io_context ioc;
    WSClient client(ioc, host, port);
    PerformanceMonitor pdh_monitor(std::chrono::seconds(5));

    pdh_monitor.set_callback([&](const std::vector<MonitoredPdhCounterData> &data_snapshot) {
        if (!client.isConnected()) {
            std::cout << "PDH Callback: WebSocket not connected. Skipping send." << std::endl;
            return;
        }
        if (data_snapshot.empty()) {
            return;
        }
        std::string json_payload = format_data_to_json(client_id, data_snapshot);
        std::cout << "PDH: Sending performance data..." << std::endl;
        client.send(json_payload);
    });

    client.setOnConnectCallback([&](boost::system::error_code ec) {
        if (ec) {
            std::cerr << "WebSocket: Failed to connect: " << ec.message() << std::endl;
            ioc.stop();
            return;
        }
        std::cout << "WebSocket: Connection successful!" << std::endl;
        std::cout << "Main: Starting performance monitoring." << std::endl;
        pdh_monitor.start_monitoring();
    });

    client.setOnMessageCallback([](const std::string &message) {
        std::cout << "WebSocket: Message received: " << message << std::endl;
    });

    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](boost::system::error_code /ec/, int /signum/) {
        std::cout << "\nSignal received. Initiating shutdown..." << std::endl;
        pdh_monitor.stop_monitoring();
        if (client.isConnected()) {
            std::cout << "Main: Disconnecting WebSocket..." << std::endl;
            client.disconnect();
        }
    });

    pdh_monitor.add_counter("CPU Usage",
        L"\\Processor Information(_Total)\\% Processor Utility");
    pdh_monitor.add_counter("Available RAM (MB)",
        L"\\Memory\\Available MBytes");

    if (!pdh_monitor.initialize()) {
        std::cerr << "Main: Failed to initialize PerformanceMonitor. Exiting." << std::endl;
        return 1;
    }

    client.connect();

    std::thread asio_thread([&ioc]() {
        std::cout << "Main: Starting Asio event loop. Press Ctrl+C to exit." << std::endl;
        ioc.run();
        std::cout << "Main: Asio event loop finished." << std::endl;
    });

    asio_thread.join();

    std::cout << "Main: Shutdown complete. Exiting." << std::endl;
    return 0;
}