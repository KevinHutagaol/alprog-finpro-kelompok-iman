#include "wsclient.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>
#include <unistd.h> // For gethostname

// Simulated function to collect performance metrics
nlohmann::json collectPerformanceMetrics() {
    // In a real implementation, this would use system APIs to collect actual metrics
    nlohmann::json metrics = {
        {"cpu_usage", rand() % 100},
        {"memory_usage", rand() % 8192},
        {"disk_io", {
            {"read_bps", rand() % 100000},
            {"write_bps", rand() % 100000}
        }},
        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
    };

    return metrics;
}

int main() {
    // Get hostname for client ID
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    std::string clientId = std::string(hostname) + "-" + std::to_string(getpid());

    // Create WebSocket client
    PerformanceWSClient client("wss://your-central-server.example.com/metrics", clientId);

    // Set up callbacks
    client.setConnectionCallback([](bool connected) {
        std::cout << "Connection status changed: " << (connected ? "Connected" : "Disconnected") << std::endl;
    });

    client.setMessageCallback([](const std::string& message) {
        std::cout << "Received message: " << message << std::endl;
    });

    client.setErrorCallback([](const std::string& error) {
        std::cerr << "Error: " << error << std::endl;
    });

    // Connect to server
    if (!client.connect()) {
        std::cerr << "Failed to connect initially, but client will retry automatically" << std::endl;
    }

    // Send metrics every 10 seconds
    while (true) {
        // Collect metrics
        auto metrics = collectPerformanceMetrics();

        // Send to server
        if (!client.sendMetrics(metrics.dump())) {
            std::cout << "Metrics queued for delivery when connection is restored" << std::endl;
        }

        // Wait before sending next metrics
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    return 0;
}