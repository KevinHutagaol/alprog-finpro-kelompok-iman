#ifndef SESSION_H
#define SESSION_H



#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>

#include <memory>
#include <string>
#include <list>

class WSServer;

class Session : public std::enable_shared_from_this<Session> {
public:
    void send(const std::string& message);

private:
    friend class WSServer;

    Session(boost::asio::ip::tcp::socket&& socket, WSServer& server);

    void run(boost::asio::yield_context yield);

    void do_read(boost::asio::yield_context yield);

    void do_write(boost::asio::yield_context yield);

    boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
    boost::beast::flat_buffer buffer_;
    std::list<std::shared_ptr<const std::string>> write_queue_;

    WSServer& server_;
};



#endif //SESSION_H
