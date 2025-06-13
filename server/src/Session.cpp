#include "Session.h"

void Session::send(const std::string &message) {
}

Session::Session(boost::asio::ip::tcp::socket &&socket, WSServer &server) {
}

void Session::run(boost::asio::yield_context yield) {
}

void Session::do_read(boost::asio::yield_context yield) {
}

void Session::do_write(boost::asio::yield_context yield) {
}
