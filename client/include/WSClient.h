#ifndef WSCLIENT_H
#define WSCLIENT_H

#include <string>
#include <functional>
#include <memory>
#include <condition_variable>

/**
 * @class WSClient
 * @brief A WebSocket client class that can be accessed by any other classes
 *
 * This class provides a generic WebSocket client implementation that can be used
 * to establish WebSocket connections, send messages, and receive messages from a WebSocket server.
 */
class WSClient {
public:
    /**
     * @brief Constructor for WSClient
     */
    WSClient();

    /**
     * @brief Destructor for WSClient
     */
    ~WSClient();

    /**
     * @brief Connect to a WebSocket server
     * @param uri The URI of the WebSocket server
     * @return true if connection is successful, false otherwise
     */
    bool connect(const std::string& uri);

    /**
     * @brief Disconnect from the WebSocket server
     */
    void disconnect();

    /**
     * @brief Send a message to the WebSocket server
     * @param message The message to send
     * @return true if message is sent successfully, false otherwise
     */
    bool send(const std::string& message);

    /**
     * @brief Set callback function for when a message is received
     * @param callback The function to call when a message is received
     */
    void setOnMessageCallback(std::function<void(const std::string&)> callback);

    /**
     * @brief Set callback function for when connection is established
     * @param callback The function to call when connection is established
     */
    void setOnConnectCallback(std::function<void()> callback);

    /**
     * @brief Set callback function for when connection is closed
     * @param callback The function to call when connection is closed
     */
    void setOnCloseCallback(std::function<void(const std::string&)> callback);

    /**
     * @brief Set callback function for when an error occurs
     * @param callback The function to call when an error occurs
     */
    void setOnErrorCallback(std::function<void(const std::string&)> callback);

    /**
     * @brief Check if the client is connected to the server
     * @return true if connected, false otherwise
     */
    bool isConnected() const;

private:
    // WebSocket internal implementation details
    class WSClientImpl;
    std::unique_ptr<WSClientImpl> pImpl;
};

#endif // WSCLIENT_H
