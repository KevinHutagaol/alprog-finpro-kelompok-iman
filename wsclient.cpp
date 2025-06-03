#include "wsclient.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <iostream>

using json = nlohmann::json;

PerformanceWSClient::PerformanceWSClient(const std::string& serverUrl, const std::string& clientId)
    : serverUrl_(serverUrl),
      clientId_(clientId),
      connected_(false),
      reconnecting_(false),
      reconnectAttempts_(0),
      running_(false) {
    
    setupWebSocket();
    
    // Start the message processing thread
    running_ = true;
    processingThread_ = std::thread(&PerformanceWSClient::processMessageQueue, this);
}

PerformanceWSClient::~PerformanceWSClient() {
    // Signal the processing thread to stop
    running_ = false;
    
    // Disconnect from the server
    disconnect();
    
    // Wait for the processing thread to finish
    if (processingThread_.joinable()) {
        processingThread_.join();
    }
}

void PerformanceWSClient::setupWebSocket() {
    // Configure WebSocket
    webSocket_.setUrl(serverUrl_);
    
    // Set ping/pong interval for keeping connection alive
    webSocket_.setPingInterval(45); // seconds
    
    // Configure callbacks
    webSocket_.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            if (messageCallback_) {
                messageCallback_(msg->str);
            }
        }
        else if (msg->type == ix::WebSocketMessageType::Open) {
            connected_ = true;
            reconnectAttempts_ = 0;
            
            if (connectionCallback_) {
                connectionCallback_(true);
            }
            
            // Send any pending messages
            std::lock_guard<std::mutex> lock(queueMutex_);
            while (!messageQueue_.empty()) {
                sendMessage(messageQueue_.front());
                messageQueue_.pop();
            }
        }
        else if (msg->type == ix::WebSocketMessageType::Close) {
            connected_ = false;
            
            if (connectionCallback_) {
                connectionCallback_(false);
            }
            
            // Try to reconnect if not manually disconnected
            if (!reconnecting_ && running_) {
                tryReconnect();
            }
        }
        else if (msg->type == ix::WebSocketMessageType::Error) {
            if (errorCallback_) {
                errorCallback_("WebSocket error: " + msg->errorInfo.reason);
            }
            
            connected_ = false;
            
            if (connectionCallback_) {
                connectionCallback_(false);
            }
            
            // Try to reconnect if not manually disconnected
            if (!reconnecting_ && running_) {
                tryReconnect();
            }
        }
    });
}

bool PerformanceWSClient::connect() {
    if (connected_) {
        return true; // Already connected
    }
    
    // Start connection
    webSocket_.start();
    return true; // Connection initiated
}

void PerformanceWSClient::disconnect() {
    webSocket_.stop();
    connected_ = false;
    reconnecting_ = false;
}

bool PerformanceWSClient::sendMetrics(const std::string& metricsJson) {
    // Create payload with client ID and timestamp
    std::string payload = createMetricsPayload(metricsJson);
    
    if (connected_) {
        return sendMessage(payload);
    }
    else {
        // Queue message for later delivery
        std::lock_guard<std::mutex> lock(queueMutex_);
        messageQueue_.push(payload);
        
        // Try to connect if not already connecting
        if (!connected_ && !reconnecting_ && running_) {
            connect();
        }
        
        return true; // Message queued successfully
    }
}

std::string PerformanceWSClient::createMetricsPayload(const std::string& metricsJson) {
    // Parse the incoming metrics JSON
    json metrics;
    try {
        metrics = json::parse(metricsJson);
    }
    catch (const std::exception& e) {
        // If parsing fails, create a new JSON object with the raw metrics
        metrics = {{"raw_metrics", metricsJson}};
    }
    
    // Add client identifier and timestamp
    metrics["client_id"] = clientId_;
    metrics["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    return metrics.dump();
}

bool PerformanceWSClient::sendMessage(const std::string& message) {
    if (!connected_) {
        return false;
    }
    
    // Send the message
    auto result = webSocket_.send(message);
    return result.success;
}

bool PerformanceWSClient::isConnected() const {
    return connected_;
}

void PerformanceWSClient::setConnectionCallback(std::function<void(bool connected)> callback) {
    connectionCallback_ = std::move(callback);
}

void PerformanceWSClient::setMessageCallback(std::function<void(const std::string& message)> callback) {
    messageCallback_ = std::move(callback);
}

void PerformanceWSClient::setErrorCallback(std::function<void(const std::string& error)> callback) {
    errorCallback_ = std::move(callback);
}

void PerformanceWSClient::processMessageQueue() {
    while (running_) {
        if (connected_ && !messageQueue_.empty()) {
            std::string message;
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                if (!messageQueue_.empty()) {
                    message = messageQueue_.front();
                    messageQueue_.pop();
                }
            }
            
            if (!message.empty()) {
                sendMessage(message);
            }
        }
        
        // Sleep to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void PerformanceWSClient::tryReconnect() {
    reconnecting_ = true;
    reconnectAttempts_++;
    
    // Exponential backoff for reconnection attempts
    int delay = std::min(30, reconnectAttempts_ * 2); // Max 30 seconds
    
    if (errorCallback_) {
        errorCallback_("Connection lost. Attempting to reconnect in " + std::to_string(delay) + " seconds...");
    }
    
    // Wait before reconnecting
    std::this_thread::sleep_for(std::chrono::seconds(delay));
    
    // Try to reconnect
    if (running_) {
        connect();
    }
    
    reconnecting_ = false;
}