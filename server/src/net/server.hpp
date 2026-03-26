#pragma once
#include <boost/asio.hpp>

#include "net/request_handler.hpp"

namespace psp {

class Server {
public:
    Server(boost::asio::io_context& ioc,
           unsigned short port,
           RequestHandler& handler);

    void run(std::size_t thread_count);
    void stop();

private:
    void do_accept();

    boost::asio::io_context&       ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
    RequestHandler&                handler_;
};

} // namespace psp
