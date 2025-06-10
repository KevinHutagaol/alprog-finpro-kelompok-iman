#include "WSClient.h"

#include <iostream>
#include <utility>


WSClient::WSClient(boost::asio::io_context &ioc_, const std::string &host_, const std::string &port_): host_(host_),
    port_(port_), resolver_(ioc_), ioc_(ioc_), ws_(ioc_) {
}

WSClient::~WSClient() {
}

void WSClient::connect() {
    boost::asio::spawn(this->ioc_, [this](boost::asio::yield_context yield) {
        boost::beast::error_code ec;

        const auto results = this->resolver_.async_resolve(this->host_, this->port_, yield[ec]);
        if (ec) {
            return fail(ec, "resolve");
        }
        boost::beast::get_lowest_layer(this->ws_).expires_after(std::chrono::seconds(30));

        const auto ep = boost::beast::get_lowest_layer(this->ws_).async_connect(results, yield[ec]);
        if (ec) {
            return fail(ec, "connect");
        }

        std::string host_with_port = this->host_ + ':' + std::to_string(ep.port());

        boost::beast::get_lowest_layer(this->ws_).expires_never();

        this->ws_.set_option(
            boost::beast::websocket::stream_base::timeout::suggested(
                boost::beast::role_type::client));

        this->ws_.set_option(boost::beast::websocket::stream_base::decorator(
            [](boost::beast::websocket::request_type &req) {
                req.set(boost::beast::http::field::user_agent,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-coro");
            }));

        this->ws_.async_handshake(host_with_port, "/", yield[ec]);
        if (ec) {
            return fail(ec, "handshake");
        }

        this->is_connected_.store(true);
        std::cout << "Connected to: " << host_with_port << std::endl;
        read_loop();

        if (this->on_connect_callback_) {
            this->on_connect_callback_({});
        }
    });
}

void WSClient::disconnect() {
    if (!this->is_connected_.load()) {
        return;
    }

    // change to ioc_ thread (which is created from connect fn)
    boost::asio::post(this->ioc_, [this]() {
        this->ws_.async_close(boost::beast::websocket::close_code::normal, [this](auto ec) {
            this->is_connected_.store(false);
            if (ec) {
                return fail(ec, "close");
            }
        });
    });
}

void WSClient::send(const std::string &message) {
    const auto shared_message = std::make_shared<std::string>(message);

    boost::asio::post(this->ioc_, [this, shared_message]() {
        this->ws_.async_write(*shared_message, [this](auto ec, auto) {
            if (this->on_send_callback_) {
                on_send_handler_(ec);
            }

            if (ec) {
                return fail(ec, "write");
            }
        });
    });
}

void WSClient::read_loop() {
    // does not need boost::asio::post because it is only called from connect
    // which is already in the thread
    this->ws_.async_read(this->buffer_, [this](auto ec, auto) {
        if (ec == boost::beast::websocket::error::closed) {
            is_connected_ = false;
            std::cout << "Connection closed by peer." << std::endl;
            return;
        }
        if (ec) {
            return fail(ec, "read");
        }
        if (this->on_message_callback_) {
            this->on_message_callback_(boost::beast::buffers_to_string(this->buffer_.data()));
        }

        this->buffer_.consume(this->buffer_.size());

        read_loop();
    });
}


bool WSClient::isConnected() const {
    return this->is_connected_.load();
}

void WSClient::setOnMessageCallback(std::function<void(const std::string &)> on_message_handler) {
    this->on_message_callback_ = std::move(on_message_handler);
}

void WSClient::setOnConnectCallback(std::function<void(const boost::beast::error_code &)> on_connect_handler) {
    this->on_connect_callback_ = std::move(on_connect_handler);
}

void WSClient::setOnSendCallback(std::function<void(const boost::beast::error_code &)> on_send_handler) {
    this->on_send_callback_ = std::move(on_send_handler);
}

void WSClient::fail(const boost::beast::error_code &ec, char const *what) {
    std::cerr << what << ": " << ec.message() << "\n";
}
