#include "wsclient.h"
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>

// JSON library for serialization
using json = nlohmann::json;

// Implement PerformanceData serialization methods
std::string PerformanceData::toJson() const {
    json j;
    j["cpuUsage"] = cpuUsage;
    j["memoryUsage"] = memoryUsage;
    j["diskIORate"] = diskIORate;
    j["timestamp"] = timestamp;
    j["clientId"] = clientId;
    return j.dump();
}

PerformanceData PerformanceData::fromJson(const std::string& jsonStr) {
    PerformanceData data;
    try {
        json j = json::parse(jsonStr);
        data.cpuUsage = j["cpuUsage"];
        data.memoryUsage = j["memoryUsage"];
        data.diskIORate = j["diskIORate"];
        data.timestamp = j["timestamp"];
        data.clientId = j["clientId"];
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
    }
    return data;
}

// WSClient implementation
WSClient::WSClient(const std::string& serverUri, const std::string& clientId, int retryIntervalMs)
    : m_serverUri(serverUri),
      m_clientId(clientId),
      m_retryIntervalMs(retryIntervalMs),
      m_connected(false),
      m_running(false) {
    m_ioService = std::make_unique<websocketpp::lib::asio::io_service>();
    initializeClient();
}

WSClient::~WSClient() {
    disconnect();

    m_running = false;

    if (m_ioThread.joinable()) {
        m_ioThread.join();
    }

    if (m_reconnectThread.joinable()) {
        m_reconnectThread.join();
    }

    if (m_queueProcessorThread.joinable()) {
        m_queueProcessorThread.join();
    }
}

void WSClient::initializeClient() {
    m_client = std::make_unique<WebsocketClient>();

    // Set logging settings
    m_client->set_access_channels(websocketpp::log::alevel::all);
    m_client->clear_access_channels(websocketpp::log::alevel::frame_payload);

    // Initialize ASIO
    m_client->init_asio(m_ioService.get());

    // Register handlers
    m_client->set_open_handler([this](websocketpp::connection_hdl hdl) {
        {
            std::lock_guard<std::mutex> lock(m_connectionMutex);
            m_connectionHandle = hdl;
            m_connected = true;
        }

        if (m_onConnectCallback) {
            m_onConnectCallback();
        }
    });

    m_client->set_close_handler([this](websocketpp::connection_hdl hdl) {
        int code = 0;
        std::string reason;

        try {
            auto conn = m_client->get_con_from_hdl(hdl);
            code = conn->get_remote_close_code();
            reason = conn->get_remote_close_reason();
        } catch (const std::exception& e) {
            reason = e.what();
        }

        {
            std::lock_guard<std::mutex> lock(m_connectionMutex);
            m_connected = false;
        }

        if (m_onCloseCallback) {
            m_onCloseCallback(code, reason);
        }
    });

    m_client->set_fail_handler([this](websocketpp::connection_hdl hdl) {
        std::string errorMsg;

        try {
            auto conn = m_client->get_con_from_hdl(hdl);
            errorMsg = conn->get_ec().message();
        } catch (const std::exception& e) {
            errorMsg = e.what();
        }

        {
            std::lock_guard<std::mutex> lock(m_connectionMutex);
            m_connected = false;
        }

        if (m_onErrorCallback) {
            m_onErrorCallback("Connection failed: " + errorMsg);
        }
    });

    m_client->set_message_handler([this](websocketpp::connection_hdl hdl,
                                         WebsocketClient::message_ptr msg) {
        if (m_onMessageCallback) {
            m_onMessageCallback(msg->get_payload());
        }
    });
}

bool WSClient::connect() {
    if (isConnected()) {
        return true;
    }

    websocketpp::lib::error_code ec;
    WebsocketClient::connection_ptr conn = m_client->get_connection(m_serverUri, ec);

    if (ec) {
        if (m_onErrorCallback) {
            m_onErrorCallback("Connect initialization error: " + ec.message());
        }
        return false;
    }

    // Add client identification header
    conn->append_header("Client-ID", m_clientId);

    try {
        m_client->connect(conn);

        // Start the ASIO io_service run loop
        if (!m_running) {
            m_running = true;

            m_ioThread = std::thread([this]() {
                while (m_running) {
                    try {
                        m_ioService->run();
                    } catch (const std::exception& e) {
                        if (m_onErrorCallback) {
                            m_onErrorCallback("IO Service error: " + std::string(e.what()));
                        }
                    }
                }
            });

            m_reconnectThread = std::thread([this]() {
                reconnectionLoop();
            });

            m_queueProcessorThread = std::thread([this]() {
                processSendQueue();
            });
        }

        return true;
    } catch (const websocketpp::exception& e) {
        if (m_onErrorCallback) {
            m_onErrorCallback("Connection error: " + std::string(e.what()));
        }
        return false;
    }
}

void WSClient::disconnect() {
    std::lock_guard<std::mutex> lock(m_connectionMutex);

    if (m_connected && !m_connectionHandle.expired()) {
        websocketpp::lib::error_code ec;
        m_client->close(m_connectionHandle, websocketpp::close::status::normal, "Client disconnecting", ec);

        if (ec) {
            if (m_onErrorCallback) {
                m_onErrorCallback("Error closing connection: " + ec.message());
            }
        }
    }

    m_connected = false;
}

bool WSClient::sendPerformanceData(const PerformanceData& data) {
    if (!isConnected()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_connectionMutex);

    if (m_connectionHandle.expired()) {
        return false;
    }

    try {
        websocketpp::lib::error_code ec;
        m_client->send(m_connectionHandle, data.toJson(), websocketpp::frame::opcode::text, ec);

        if (ec) {
            if (m_onErrorCallback) {
                m_onErrorCallback("Error sending data: " + ec.message());
            }
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        if (m_onErrorCallback) {
            m_onErrorCallback("Send error: " + std::string(e.what()));
        }
        return false;
    }
}

void WSClient::queuePerformanceData(const PerformanceData& data) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_sendQueue.push(data);
}

bool WSClient::isConnected() const {
    return m_connected;
}

void WSClient::setOnConnectCallback(std::function<void()> callback) {
    m_onConnectCallback = std::move(callback);
}

void WSClient::setOnCloseCallback(std::function<void(int, const std::string&)> callback) {
    m_onCloseCallback = std::move(callback);
}

void WSClient::setOnMessageCallback(std::function<void(const std::string&)> callback) {
    m_onMessageCallback = std::move(callback);
}

void WSClient::setOnErrorCallback(std::function<void(const std::string&)> callback) {
    m_onErrorCallback = std::move(callback);
}

void WSClient::processSendQueue() {
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        PerformanceData data;
        bool hasData = false;

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (!m_sendQueue.empty()) {
                data = m_sendQueue.front();
                m_sendQueue.pop();
                hasData = true;
            }
        }

        if (hasData) {
            if (!sendPerformanceData(data)) {
                // Failed to send, put it back in the queue
                std::lock_guard<std::mutex> lock(m_queueMutex);
                m_sendQueue.push(data);
            }
        }
    }
}

void WSClient::reconnectionLoop() {
    while (m_running) {
        if (!isConnected()) {
            connect();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(m_retryIntervalMs));
    }
}