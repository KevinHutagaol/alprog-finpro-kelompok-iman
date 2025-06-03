#pragma once

#include <string>
#include <functional>
#include <mutex>
#include <queue>
#include <atomic>
#include <thread>
#include <ixwebsocket/IXWebSocket.h>

/**
 * @class PerformanceWSClient
 * @brief WebSocket client for sending computer performance metrics to a central server
 */
class PerformanceWSClient {
public:
    /**
     * @brief Constructor
     * @param serverUrl The WebSocket server URL (e.g., "wss://central-server.example.com/metrics")
     * @param clientId Unique identifier for this client (e.g., computer name or lab ID)
     */
    PerformanceWSClient(const std::string& serverUrl, const std::string& clientId);
    
    /**
     * @brief Destructor - ensures proper cleanup of resources
     */
    ~PerformanceWSClient();

    /**
     * @brief Connect to the WebSocket server
     * @return True if connection initiated successfully, false otherwise
     */
    bool connect();

    /**
     * @brief Disconnect from the WebSocket server
     */
    void disconnect();

    /**
     * @brief Send performance metrics to the server
     * @param metricsJson JSON string containing the metrics data
     * @return True if the message was queued successfully, false otherwise
     */
    bool sendMetrics(const std::string& metricsJson);

    /**
     * @brief Check if the client is currently connected
     * @return True if connected, false otherwise
     */
    bool isConnected() const;

    /**
     * @brief Set callback for connection events
     * @param callback Function to call when connection state changes
     */
    void setConnectionCallback(std::function<void(bool connected)> callback);
    
    /**
     * @brief Set callback for message events
     * @param callback Function to call when a message is received
     */
    void setMessageCallback(std::function<void(const std::string& message)> callback);
    
    /**
     * @brief Set callback for error events
     * @param callback Function to call when an error occurs
     */
    void setErrorCallback(std::function<void(const std::string& error)> callback);

private:
    ix::WebSocket webSocket_;
    std::string serverUrl_;
    std::string clientId_;
    
    std::atomic<bool> connected_;
    std::atomic<bool> reconnecting_;
    std::atomic<int> reconnectAttempts_;
    
    std::mutex queueMutex_;
    std::queue<std::string> messageQueue_;
    std::thread processingThread_;
    std::atomic<bool> running_;
    
    std::function<void(bool connected)> connectionCallback_;
    std::function<void(const std::string& message)> messageCallback_;
    std::function<void(const std::string& error)> errorCallback_;
    
    // Internal methods
    void setupWebSocket();
    void processMessageQueue();
    void tryReconnect();
    bool sendMessage(const std::string& message);
    std::string createMetricsPayload(const std::string& metricsJson);
};