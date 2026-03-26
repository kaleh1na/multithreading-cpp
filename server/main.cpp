#include <iostream>
#include <thread>
#include <boost/asio.hpp>

#include "store/account_store.hpp"
#include "store/transaction_store.hpp"
#include "service/payment_service.hpp"
#include "net/request_handler.hpp"
#include "net/server.hpp"

int main(int argc, char* argv[]) {
    const unsigned short port = (argc > 1)
        ? static_cast<unsigned short>(std::stoul(argv[1]))
        : 8080;
    const std::size_t threads = std::max(1u, std::thread::hardware_concurrency());

    AccountStore     accounts;
    TransactionStore transactions;
    PaymentService   service(accounts, transactions);
    psp::RequestHandler handler(service);

    boost::asio::io_context ioc;
    psp::Server server(ioc, port, handler);

    std::cout << "PSP server listening on port " << port
              << " (" << threads << " threads)\n";

    server.run(threads);
    return 0;
}
