#ifndef WSSERVER_H
#define WSSERVER_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>

#include <functional>
#include <memory>
#include <string>
#include <list>
#include <mutex>

#include "Session.h"


class WSServer {
public:
    WSServer(boost::asio::io_context &ioc, unsigned short port);

    void run();

    void broadcast(const std::string &message);

    void setOnConnectCallback(std::function<void(std::shared_ptr<Session>)> on_connect_callback);

    void setOnDisconnectCallback(std::function<void(std::shared_ptr<Session>)> on_disconnect_callback);

    void setOnMessageCallback(std::function<void(std::shared_ptr<Session>, const std::string &)> on_message_callback);

private:
    friend class Session;

    void do_accept(boost::asio::yield_context yield);

    void join(std::shared_ptr<Session> session);

    void leave(std::shared_ptr<Session> session);

    boost::asio::io_context &ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;

    std::list<std::shared_ptr<Session> > sessions_;
    std::mutex sessions_mutex_;

    std::function<void(std::shared_ptr<Session>)> on_connect_callback_;
    std::function<void(std::shared_ptr<Session>)> on_disconnect_callback_;
    std::function<void(std::shared_ptr<Session>, const std::string &)> on_message_callback_;
};

#endif //WSSERVER_H
