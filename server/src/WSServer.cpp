#include "WSServer.h"

#include <iostream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

WSServer::WSServer(boost::asio::io_context &ioc, unsigned short port): ioc_(ioc),
                                                                       acceptor_(ioc, {tcp::v4(), port}) {
}

void WSServer::run() {
    net::spawn(acceptor_.get_executor(), [this](net::yield_context yield) {
        this->do_accept(yield);
    });
}

void WSServer::broadcast(const std::string &message) {
    auto const shared_msg = std::make_shared<const std::string>(message);
    std::list<std::shared_ptr<Session>> sessions_copy;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_copy = sessions_;
    }
    for (auto const& session : sessions_copy) {
        session->send(*shared_msg);
    }
}

void WSServer::setOnConnectCallback(std::function<void(std::shared_ptr<Session>)> on_connect_callback) {
    on_connect_callback_ = std::move(on_connect_callback);
}

void WSServer::setOnDisconnectCallback(std::function<void(std::shared_ptr<Session>)> on_disconnect_callback) {
    on_disconnect_callback_ = std::move(on_disconnect_callback);
}

void WSServer::setOnMessageCallback(std::function<void(std::shared_ptr<Session>, const std::string&)> on_message_callback) {
    on_message_callback_ = std::move(on_message_callback);
}

void WSServer::do_accept(boost::asio::yield_context yield) {
    beast::error_code ec;
    for (;;) {
        tcp::socket socket(ioc_);
        acceptor_.async_accept(socket, yield[ec]);
        if (ec) {
            std::cerr << "Accept failed: " << ec.message() << std::endl;
        } else {
            auto session = Session::create(std::move(socket), *this);
            net::spawn(acceptor_.get_executor(), [session](net::yield_context yield) {
                session->run(yield);
            });
        }
    }
}

void WSServer::join(std::shared_ptr<Session> session) {
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_.emplace_back(session);
    }
    if (on_connect_callback_) {
        on_connect_callback_(session);
    }
}

void WSServer::leave(std::shared_ptr<Session> session) {
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_.remove(session);
    }
    if (on_disconnect_callback_) {
        on_disconnect_callback_(session);}
}