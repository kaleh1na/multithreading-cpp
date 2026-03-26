#pragma once
#include <boost/asio.hpp>
#include <memory>

#include "net/request_handler.hpp"

namespace psp {

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket socket, RequestHandler& handler);
    void start();

private:
    void do_read();
    void do_write(std::string response);

    boost::asio::ip::tcp::socket socket_;
    boost::asio::streambuf        buffer_;
    RequestHandler&               handler_;
};

} // namespace psp
