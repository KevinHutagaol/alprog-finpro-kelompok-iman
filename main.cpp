#include "wsclient.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <random>

// Example function to simulate getting performance data
PerformanceData getPerformanceMetrics(const std::string& clientId) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> cpuDist(0.0, 100.0);
    static std::uniform_real_distribution<> memDist(0.0, 32.0); // GB
    static std::uniform_real_distribution<> ioDist(0.0, 200.0); // MB/s

    PerformanceData data;
    data.cpuUsage = cpuDist(gen);
    data.memoryUsage = memDist(gen);
    data.diskIORate = ioDist(gen);
    data.clientId = clientId;

    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    data.timestamp = ss.str();

    return data;
}

int main() {
    // Create a unique client ID based on computer name or other identifier
    std::string clientId = "LabPC-" + std::to_string(std::random_device{}());

    // Create WebSocket client
    WSClient client("ws://performance-server.example.com:8080/ws", clientId);

    // Set callbacks
    client.setOnConnectCallback([]() {
        std::cout << "Connected to server" << std::endl;
    });

    client.setOnCloseCallback([](int code, const std::string& reason) {
        std::cout << "Connection closed: " << code << " - " << reason << std::endl;
    });

    client.setOnMessageCallback([](const std::string& message) {
        std::cout << "Received message: " << message << std::endl;
    });

    client.setOnErrorCallback([](const std::string& error) {
        std::cerr << "Error: " << error << std::endl;
    });

    // Connect to server
    if (!client.connect()) {
        std::cerr << "Failed to initiate connection" << std::endl;
    }

    // Main loop - collect and send performance data every 5 seconds
    while (true) {
        try {
            // Get current performance metrics
            PerformanceData metrics = getPerformanceMetrics(clientId);

            // Queue metrics to be sent
            client.queuePerformanceData(metrics);

            // Debug output
            std::cout << "Queued metrics - CPU: " << metrics.cpuUsage
                      << "%, Memory: " << metrics.memoryUsage
                      << "GB, I/O: " << metrics.diskIORate << "MB/s" << std::endl;

            // Wait for 5 seconds before collecting next batch of metrics
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        catch (const std::exception& e) {
            std::cerr << "Error in main loop: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}