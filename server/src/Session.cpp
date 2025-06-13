#include "Session.h"

#include "WSServer.h"
#include <iostream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

void Session::send(const std::string &message) {
    auto shared_msg = std::make_shared<const std::string>(message);

    net::post(
        ws_.get_executor(),
        [self = shared_from_this(), shared_msg]() {
            self->write_queue_.push_back(shared_msg);

            if (self->write_queue_.size() > 1) {
                return;
            }

            net::spawn(self->ws_.get_executor(), std::bind(&Session::do_write, self, std::placeholders::_1));
        });
}


Session::Session(boost::asio::ip::tcp::socket &&socket, WSServer &server): ws_(std::move(socket)), server_(server) {
}

void Session::run(net::yield_context yield) {
    beast::error_code ec;


    try {
        ws_.async_accept(yield[ec]);
        if (ec) {
            std::cerr << "Handshake failed: " << ec.message() << std::endl;
            return;
        }

        server_.join(shared_from_this());

        do_read(yield);
    } catch (const std::exception &e) {
        std::cerr << "Session error: " << e.what() << std::endl;
    }

    server_.leave(shared_from_this());

    ws_.async_close(websocket::close_code::normal, yield[ec]);
    if (ec) {
        std::cerr << "Close failed: " << ec.message() << std::endl;
    }
}

void Session::do_read(boost::asio::yield_context yield) {
    beast::error_code ec;

    for (;;) {
        ws_.async_read(buffer_, yield[ec]);

        if (ec == websocket::error::closed) {
            break;
        }

        if (ec) {
            std::cerr << "Read failed: " << ec.message() << std::endl;
            break;
        }

        if (server_.on_message_callback_) {
            auto message = beast::buffers_to_string(buffer_.data());
            server_.on_message_callback_(shared_from_this(), message);
        }

        buffer_.consume(buffer_.size());
    }
}

void Session::do_write(boost::asio::yield_context yield) {
    beast::error_code ec;

    // While there are messages in the queue...
    while (!write_queue_.empty()) {
        ws_.text(true);
        ws_.async_write(net::buffer(*write_queue_.front()), yield[ec]);

        if (ec) {
            std::cerr << "Write failed: " << ec.message() << std::endl;
            write_queue_.clear();
            return;
        }

        write_queue_.pop_front();}
}

boost::asio::ip::tcp::endpoint Session::get_remote_endpoint() const {
    return beast::get_lowest_layer(ws_).socket().remote_endpoint();
}
    beast::error_code ec;

    // While there are messages in the queue...
    while (!write_queue_.empty()) {
        ws_.text(true);
        ws_.async_write(net::buffer(*write_queue_.front()), yield[ec]);

        if (ec) {
            std::cerr << "Write failed: " << ec.message() << std::endl;
            write_queue_.clear();
            return;
        }

        write_queue_.pop_front();}
}

boost::asio::ip::tcp::endpoint Session::get_remote_endpoint() const {
    return beast::get_lowest_layer(ws_).socket().remote_endpoint();
}