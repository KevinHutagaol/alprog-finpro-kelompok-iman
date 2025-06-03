#ifndef WSCLIENT_H
#define WSCLIENT_H

#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>

// Forward declarations for websocketpp to avoid including the entire library in the header
namespace websocketpp {
    namespace lib {
        namespace asio {
            class io_service;
        }
    }

    namespace config {
        struct asio_client;
    }

    template <typename T>
    class client;

    namespace connection_hdl_tag {
        struct type;
    }

    typedef std::weak_ptr<void> connection_hdl;
}

// Structure to hold performance metrics
struct PerformanceData {
    double cpuUsage;
    double memoryUsage;
    double diskIORate;
    std::string timestamp;
    std::string clientId;

    // Serialize to JSON string
    std::string toJson() const;

    // Deserialize from JSON string
    static PerformanceData fromJson(const std::string& json);
};

class WSClient {
public:
    // Constructor takes server URI, client ID, and connection retry interval
    WSClient(const std::string& serverUri, const std::string& clientId, int retryIntervalMs = 5000);

    // Destructor
    ~WSClient();

    // Connect to the WebSocket server
    bool connect();

    // Disconnect from the WebSocket server
    void disconnect();

    // Send performance data to the server
    bool sendPerformanceData(const PerformanceData& data);

    // Queue performance data to be sent
    void queuePerformanceData(const PerformanceData& data);

    // Check if the client is connected
    bool isConnected() const;

    // Set callback for connection established
    void setOnConnectCallback(std::function<void()> callback);

    // Set callback for connection closed
    void setOnCloseCallback(std::function<void(int, const std::string&)> callback);

    // Set callback for message received
    void setOnMessageCallback(std::function<void(const std::string&)> callback);

    // Set callback for error occurred
    void setOnErrorCallback(std::function<void(const std::string&)> callback);

private:
    // WebSocket client type
    typedef websocketpp::client<websocketpp::config::asio_client> WebsocketClient;

    // Private methods
    void initializeClient();
    void processSendQueue();
    void reconnectionLoop();

    // Private member variables
    std::string m_serverUri;
    std::string m_clientId;
    int m_retryIntervalMs;

    std::unique_ptr<WebsocketClient> m_client;
    websocketpp::connection_hdl m_connectionHandle;
    std::unique_ptr<websocketpp::lib::asio::io_service> m_ioService;

    std::thread m_ioThread;
    std::thread m_reconnectThread;
    std::thread m_queueProcessorThread;

    std::atomic<bool> m_connected;
    std::atomic<bool> m_running;

    std::queue<PerformanceData> m_sendQueue;
    std::mutex m_queueMutex;
    std::mutex m_connectionMutex;

    // Callback functions
    std::function<void()> m_onConnectCallback;
    std::function<void(int, const std::string&)> m_onCloseCallback;
    std::function<void(const std::string&)> m_onMessageCallback;
    std::function<void(const std::string&)> m_onErrorCallback;
};

#endif // WSCLIENT_H