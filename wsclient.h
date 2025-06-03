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
 *
 * This class handles secure metric transmission with automatic reconnection,
 * message queuing, and thread-safe operations for lab monitoring systems.
 */
class PerformanceWSClient {
public:
    /**
     * @brief Constructor
     * @param serverUrl The WebSocket server URL (e.g., "wss://central-server.example.com/metrics")
     * @param clientId Unique identifier for this client (typically hostname or lab PC ID)
     */
    PerformanceWSClient(const std::string& serverUrl, const std::string& clientId);

    /**
     * @brief Destructor - ensures proper cleanup of resources and background threads
     */
    ~PerformanceWSClient();

    // Prevent copying or moving
    PerformanceWSClient(const PerformanceWSClient&) = delete;
    PerformanceWSClient& operator=(const PerformanceWSClient&) = delete;
    PerformanceWSClient(PerformanceWSClient&&) = delete;
    PerformanceWSClient& operator=(PerformanceWSClient&&) = delete;

    /**
     * @brief Connect to the WebSocket server
     * @return True if connection initiated successfully, false otherwise
     * @note Even if initial connection fails, automatic reconnection will be attempted
     */
    bool connect();

    /**
     * @brief Disconnect from the WebSocket server
     */
    void disconnect();

    /**
     * @brief Send performance metrics to the server
     * @param metricsJson JSON string containing the metrics data
     * @return True if the message was sent or queued successfully, false on error
     * @note If disconnected, the message will be queued for delivery upon reconnection
     */
    bool sendMetrics(const std::string& metricsJson);

    /**
     * @brief Check if the client is currently connected
     * @return True if connected, false otherwise
     */
    bool isConnected() const;

    /**
     * @brief Set callback for connection state changes
     * @param callback Function to call when connection state changes (true=connected, false=disconnected)
     */
    void setConnectionCallback(std::function<void(bool connected)> callback);

    /**
     * @brief Set callback for received messages
     * @param callback Function to call when a message is received from the server
     */
    void setMessageCallback(std::function<void(const std::string& message)> callback);

    /**
     * @brief Set callback for error events
     * @param callback Function to call when an error occurs
     */
    void setErrorCallback(std::function<void(const std::string& error)> callback);

private:
    // Core WebSocket
    ix::WebSocket webSocket_;
    std::string serverUrl_;
    std::string clientId_;

    // Connection state
    std::atomic<bool> connected_;
    std::atomic<bool> reconnecting_;
    std::atomic<int> reconnectAttempts_;

    // Message queue for handling disconnections
    std::mutex queueMutex_;
    std::queue<std::string> messageQueue_;

    // Background processing
    std::thread processingThread_;
    std::atomic<bool> running_;

    // Callbacks
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