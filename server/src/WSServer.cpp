#include "WSServer.h"

WSServer::WSServer(boost::asio::io_context &ioc, unsigned short port) {
}

void WSServer::run() {
}

void WSServer::broadcast(const std::string &message) {
}

void WSServer::setOnConnectCallback(std::function<void(std::shared_ptr<Session>)> on_connect_callback) {
}

void WSServer::setOnDisconnectCallback(std::function<void(std::shared_ptr<Session>)> on_disconnect_callback) {
}

void WSServer::setOnMessageCallback(
    std::function<void(std::shared_ptr<Session>, const std::string &)> on_message_callback) {
}

void WSServer::do_accept(boost::asio::yield_context yield) {
}

void WSServer::join(std::shared_ptr<Session> session) {
}

void WSServer::leave(std::shared_ptr<Session> session) {
}
