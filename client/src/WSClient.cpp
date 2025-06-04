#include "WSClient.h"
#include <iostream>


class WSClient::WSClientImpl {
public:
    WSClientImpl() : connected(false) {}

    bool connect(const std::string& uri) {
        std::cout << "Connecting to: " << uri << std::endl;
        connected = true;
        if (onConnectCallback) {
            onConnectCallback();
        }
        return true;
    }

    void disconnect() {
        if (connected) {
            connected = false;
            if (onCloseCallback) {
                onCloseCallback("Connection closed normally");
            }
        }
    }

    bool send(const std::string& message) {
        if (!connected) {
            if (onErrorCallback) {
                onErrorCallback("Cannot send message: not connected");
            }
            return false;
        }

        std::cout << "Sending message: " << message << std::endl;
        return true;
    }

    bool isConnected() const {
        return connected;
    }

    void setOnMessageCallback(std::function<void(const std::string&)> callback) {
        onMessageCallback = callback;
    }

    void setOnConnectCallback(std::function<void()> callback) {
        onConnectCallback = callback;
    }

    void setOnCloseCallback(std::function<void(const std::string&)> callback) {
        onCloseCallback = callback;
    }

    void setOnErrorCallback(std::function<void(const std::string&)> callback) {
        onErrorCallback = callback;
    }

private:
    std::atomic<bool> connected;
    std::function<void(const std::string&)> onMessageCallback;
    std::function<void()> onConnectCallback;
    std::function<void(const std::string&)> onCloseCallback;
    std::function<void(const std::string&)> onErrorCallback;
};

WSClient::WSClient() : pImpl(std::make_unique<WSClientImpl>()) {}

WSClient::~WSClient() = default;

bool WSClient::connect(const std::string& uri) {
    return pImpl->connect(uri);
}

void WSClient::disconnect() {
    pImpl->disconnect();
}

bool WSClient::send(const std::string& message) {
    return pImpl->send(message);
}

void WSClient::setOnMessageCallback(std::function<void(const std::string&)> callback) {
    pImpl->setOnMessageCallback(callback);
}

void WSClient::setOnConnectCallback(std::function<void()> callback) {
    pImpl->setOnConnectCallback(callback);
}

void WSClient::setOnCloseCallback(std::function<void(const std::string&)> callback) {
    pImpl->setOnCloseCallback(callback);
}

void WSClient::setOnErrorCallback(std::function<void(const std::string&)> callback) {
    pImpl->setOnErrorCallback(callback);
}

bool WSClient::isConnected() const {
    return pImpl->isConnected();
}
