#include "wsclient.h"
#include <iostream>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

PerformanceWSClient::PerformanceWSClient(const std::string& serverUrl, const std::string& clientId)
    : serverUrl_(serverUrl),
      clientId_(clientId),
      connected_(false),
      reconnecting_(false),
      reconnectAttempts_(0),
      running_(true) {

    setupWebSocket();

    // Start message processing thread
    processingThread_ = std::thread(&PerformanceWSClient::processMessageQueue, this);
}

PerformanceWSClient::~PerformanceWSClient() {
    // Stop background thread and cleanup
    running_ = false;

    if (processingThread_.joinable()) {
        processingThread_.join();
    }

    disconnect();
}

void PerformanceWSClient::setupWebSocket() {
    // Setup WebSocket callbacks
    webSocket_.setUrl(serverUrl_);

    // Set connection timeout to 5 seconds
    webSocket_.setHandshakeTimeoutMs(5000);

    // Setup WebSocket event handlers
    webSocket_.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            if (messageCallback_) {
                messageCallback_(msg->str);
            }
        } else if (msg->type == ix::WebSocketMessageType::Open) {
            connected_ = true;
            reconnectAttempts_ = 0;

            if (connectionCallback_) {
                connectionCallback_(true);
            }

            // Process any queued messages that accumulated while disconnected
            std::lock_guard<std::mutex> lock(queueMutex_);
            while (!messageQueue_.empty() && connected_) {
                sendMessage(messageQueue_.front());
                messageQueue_.pop();
            }
        } else if (msg->type == ix::WebSocketMessageType::Close) {
            connected_ = false;

            if (connectionCallback_) {
                connectionCallback_(false);
            }

            // Try to reconnect if not explicitly disconnected
            if (running_ && !reconnecting_) {
                reconnecting_ = true;
                tryReconnect();
            }
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            if (errorCallback_) {
                errorCallback_("WebSocket error: " + msg->errorInfo.reason);
            }

            connected_ = false;

            // Try to reconnect on error
            if (running_ && !reconnecting_) {
                reconnecting_ = true;
                tryReconnect();
            }
        }
    });

    // Setup ping/pong for keeping connection alive
    webSocket_.setPingInterval(45); // Send ping every 45 seconds
}

bool PerformanceWSClient::connect() {
    if (connected_) return true;

    std::cout << "Connecting to server: " << serverUrl_ << std::endl;

    // Start connection
    webSocket_.start();

    // Wait a moment to see if connection establishes
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return webSocket_.getReadyState() == ix::ReadyState::Open;
}

void PerformanceWSClient::disconnect() {
    if (!connected_) return;

    std::cout << "Disconnecting from server" << std::endl;
    webSocket_.stop();
    connected_ = false;
}

bool PerformanceWSClient::sendMetrics(const std::string& metricsJson) {
    // Create a properly formatted metrics payload with client ID and timestamp
    std::string payload = createMetricsPayload(metricsJson);

    // If connected, send directly; otherwise, queue the message
    if (connected_) {
        return sendMessage(payload);
    } else {
        // Queue the message for later delivery
        std::lock_guard<std::mutex> lock(queueMutex_);
        messageQueue_.push(payload);

        // Try to reconnect if not already reconnecting
        if (!reconnecting_ && running_) {
            reconnecting_ = true;
            tryReconnect();
        }

        return true; // Message was queued successfully
    }
}

bool PerformanceWSClient::isConnected() const {
    return connected_;
}

void PerformanceWSClient::setConnectionCallback(std::function<void(bool connected)> callback) {
    connectionCallback_ = callback;
}

void PerformanceWSClient::setMessageCallback(std::function<void(const std::string& message)> callback) {
    messageCallback_ = callback;
}

void PerformanceWSClient::setErrorCallback(std::function<void(const std::string& error)> callback) {
    errorCallback_ = callback;
}

void PerformanceWSClient::processMessageQueue() {
    while (running_) {
        // Check if we're connected and have messages to send
        if (connected_) {
            std::lock_guard<std::mutex> lock(queueMutex_);
            while (!messageQueue_.empty() && connected_) {
                sendMessage(messageQueue_.front());
                messageQueue_.pop();
            }
        }

        // Sleep to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void PerformanceWSClient::tryReconnect() {
    const int maxRetries = 10;
    const int initialDelay = 1000; // 1 second

    while (running_ && !connected_ && reconnectAttempts_ < maxRetries) {
        reconnectAttempts_++;

        // Calculate exponential backoff delay (1s, 2s, 4s, 8s, etc.)
        int delay = initialDelay * (1 << (reconnectAttempts_ - 1));
        if (delay > 30000) delay = 30000; // Cap at 30 seconds

        std::cout << "Reconnect attempt " << reconnectAttempts_
                  << " in " << delay << "ms" << std::endl;

        // Wait before reconnecting
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));

        if (running_ && !connected_) {
            // Try to reconnect
            webSocket_.start();

            // Wait a moment to see if connection establishes
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    reconnecting_ = false;

    // If still not connected after max retries
    if (!connected_ && reconnectAttempts_ >= maxRetries) {
        if (errorCallback_) {
            errorCallback_("Failed to reconnect after maximum attempts");
        }
    }
}

bool PerformanceWSClient::sendMessage(const std::string& message) {
    if (!connected_) return false;

    bool success = webSocket_.send(message).success;

    if (!success && errorCallback_) {
        errorCallback_("Failed to send message");
    }

    return success;
}

std::string PerformanceWSClient::createMetricsPayload(const std::string& metricsJson) {
    // Parse the incoming metrics JSON
    json metrics;
    try {
        metrics = json::parse(metricsJson);
    } catch (const std::exception& e) {
        // If parsing fails, create a new JSON object with the raw string
        metrics = {{"raw_data", metricsJson}};
    }

    // Create the final payload with metadata
    json payload = {
        {"client_id", clientId_},
        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
        {"metrics", metrics},
        {"sent_at", "2025-06-03T14:26:52Z"},  // Using the current time you provided
        {"sender", "natanojsihan"}            // Using the login you provided
    };

    return payload.dump();
}