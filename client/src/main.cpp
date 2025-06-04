#include "WSClient.h"
#include "PerformanceMonitor.h"

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

std::atomic g_shutdown_flag(false);
WSClient g_web_socket_client;
std::string g_client_id;

std::string format_data_to_json(const std::vector<MonitoredPdhCounterData>& data_points) {
    if (g_client_id.empty()) {
        std::cerr << "Error: Client ID is empty in format_data_to_json." << std::endl;
    }

    std::stringstream ss;
    std::chrono::system_clock::time_point overall_timestamp = std::chrono::system_clock::now();
    for(const auto& dp : data_points){
        if(dp.hCounter != nullptr && dp.pdhStatus == ERROR_SUCCESS){
            overall_timestamp = dp.timestamp;
            break;
        }
    }

    const std::time_t overall_time_t = std::chrono::system_clock::to_time_t(overall_timestamp);
    std::tm overall_tm;
    localtime_s(&overall_tm, &overall_time_t);

    ss << "{";
    ss << "\"clientId\":\"" << g_client_id << "\",";
    ss << "\"timestamp\":\"" << std::put_time(&overall_tm, "%Y-%m-%dT%H:%M:%SZ") << "\",";
    ss << "\"counters\":[";
    bool first_counter = true;
    for (const auto& dp : data_points) {
        if (dp.hCounter == nullptr || dp.pdhStatus != ERROR_SUCCESS) continue;

        if (!first_counter) {
            ss << ",";
        }
        ss << "{";
        ss << "\"name\":\"" << dp.counter_name << "\",";
        ss << "\"value\":" << std::fixed << std::setprecision(4) << dp.counter_value;
        ss << "}";
        first_counter = false;
    }
    ss << "]";
    ss << "}";
    return ss.str();
}

void on_performance_data_ready(const std::vector<MonitoredPdhCounterData>& data_snapshot) {
    if (!g_web_socket_client.isConnected()) {
        std::cout << "PDH Callback: WebSocket not connected. Skipping send." << std::endl;
        return;
    }

    if (data_snapshot.empty()) {
        std::cout << "PDH Callback: No data in snapshot to send." << std::endl;
        return;
    }

    std::string json_payload = format_data_to_json(data_snapshot);
    std::cout << "PDH Callback: Attempting to send: " << json_payload.substr(0,100) << "..." << std::endl;

    if (!g_web_socket_client.send(json_payload)) {
        std::cerr << "PDH Callback: Failed to send message via WebSocket." << std::endl;
    } else {
        std::cout << "PDH Callback: Message sent successfully." << std::endl;
    }
}

void on_ws_connect() {
    std::cout << "WebSocket: Connected!" << std::endl;
}

void on_ws_message(const std::string& message) {
    std::cout << "WebSocket: Message received: " << message << std::endl;
}

void on_ws_close(const std::string& reason) {
    std::cout << "WebSocket: Connection closed. Reason: " << reason << std::endl;
}

void on_ws_error(const std::string& error_message) {
    std::cerr << "WebSocket: Error: " << error_message << std::endl;
}

void signal_handler_main(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down..." << std::endl;
    g_shutdown_flag.store(true);
}


int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler_main);
    signal(SIGTERM, signal_handler_main);

    std::cout << "Enter Client ID: ";
    if (!std::getline(std::cin, g_client_id) || g_client_id.empty()) {
        std::cerr << "Invalid or empty Client ID entered. Exiting." << std::endl;
        return 1;
    }

    std::cout << "Using Client ID: " << g_client_id << std::endl;

    PerformanceMonitor pdh_monitor(std::chrono::seconds(5));
    pdh_monitor.set_callback(on_performance_data_ready);

    pdh_monitor.add_counter("CPU Usage", L"\\Processor(_Total)\\% Processor Time");
    pdh_monitor.add_counter("Available RAM (MB)", L"\\Memory\\Available MBytes");

    if (!pdh_monitor.initialize()) {
        std::cerr << "Main: Failed to initialize PerformanceMonitor. Exiting." << std::endl;
        return 1;
    }

    g_web_socket_client.setOnConnectCallback(on_ws_connect);
    g_web_socket_client.setOnMessageCallback(on_ws_message);
    g_web_socket_client.setOnCloseCallback(on_ws_close);
    g_web_socket_client.setOnErrorCallback(on_ws_error);

    std::string server_uri = "ws://localhost:9000/performance";
    if (argc > 1) {
        server_uri = argv[1];
    }
    std::cout << "Main: Attempting to connect to WebSocket server at " << server_uri << std::endl;
    if (!g_web_socket_client.connect(server_uri)) {
        std::cerr << "Main: Failed to connect to WebSocket server. Exiting." << std::endl;
        return 1;
    }

    pdh_monitor.start_monitoring();
    std::cout << "Main: Performance monitoring started. Waiting for data and Ctrl+C to exit." << std::endl;

    while (!g_shutdown_flag.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Main: Initiating shutdown sequence..." << std::endl;
    pdh_monitor.stop_monitoring();
    g_web_socket_client.disconnect();

    std::cout << "Main: Shutdown complete. Exiting." << std::endl;
    return 0;
}