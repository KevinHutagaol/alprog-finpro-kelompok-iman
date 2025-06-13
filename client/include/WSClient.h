#ifndef WSCLIENT_H
#define WSCLIENT_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/spawn.hpp>
#include <string>

class WSClient {
public:
    WSClient(boost::asio::io_context &ioc, const std::string &host_, const std::string &port_);

    ~WSClient();

    void connect();

    void disconnect();

    void send(const std::string &message);

    [[nodiscard]] bool isConnected() const;

    void setOnMessageCallback(std::function<void(const std::string &)> on_message_callback);

    void setOnConnectCallback(std::function<void(const boost::beast::error_code &)> on_connect_callback);

    void setOnSendCallback(std::function<void(const boost::beast::error_code &)> on_send_callback);

private:
    std::function<void(const std::string &)> on_message_callback_;
    std::function<void(const boost::beast::error_code &)> on_connect_callback_;
    std::function<void(const boost::beast::error_code &)> on_send_callback_;
    std::mutex callbacks_mutex_;

    std::string host_;
    std::string port_;

    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::io_context &ioc_;
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
    boost::beast::flat_buffer buffer_;

    std::atomic<bool> is_connected_{false};

    void fail(const boost::beast::error_code &ec, char const *what);

    void read_loop();
};

#endif // WSCLIENT_H