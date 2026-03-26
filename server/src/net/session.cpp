#include "net/session.hpp"

namespace psp {

Session::Session(boost::asio::ip::tcp::socket socket, RequestHandler& handler)
    : socket_(std::move(socket))
    , handler_(handler) {}

void Session::start() {
    do_read();
}

void Session::do_read() {
    auto self = shared_from_this();
    boost::asio::async_read_until(
        socket_, buffer_, '\n',
        [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (ec) return;

            std::istream stream(&buffer_);
            std::string line;
            std::getline(stream, line);

            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            auto response = handler_.handle(line);
            do_write(std::move(response));
        });
}

void Session::do_write(std::string response) {
    auto self = shared_from_this();
    auto buf = std::make_shared<std::string>(std::move(response));
    boost::asio::async_write(
        socket_, boost::asio::buffer(*buf),
        [this, self, buf](const boost::system::error_code& ec, std::size_t) {
            if (ec) return;
            do_read();
        });
}

} // namespace psp
