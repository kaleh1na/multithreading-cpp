#include "net/server.hpp"
#include "net/session.hpp"

#include <thread>
#include <vector>

namespace psp {

Server::Server(boost::asio::io_context& ioc,
               unsigned short port,
               RequestHandler& handler)
    : ioc_(ioc)
    , acceptor_(ioc, boost::asio::ip::tcp::endpoint(
          boost::asio::ip::tcp::v4(), port))
    , handler_(handler)
{
    do_accept();
}

void Server::run(std::size_t thread_count) {
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (std::size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back([this] { ioc_.run(); });
    }
    for (auto& t : threads) {
        t.join();
    }
}

void Server::stop() {
    ioc_.stop();
}

void Server::do_accept() {
    acceptor_.async_accept(
        [this](const boost::system::error_code& ec,
               boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket), handler_)->start();
            }
            do_accept();
        });
}

} // namespace psp
