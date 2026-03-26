#pragma once
#include <string>
#include <vector>

#include "service/payment_service.hpp"

namespace psp {

class RequestHandler {
public:
    explicit RequestHandler(PaymentService& service);
    std::string handle(const std::string& request);

private:
    PaymentService& service_;

    std::string handle_create_account(const std::vector<std::string>& parts);
    std::string handle_deposit(const std::vector<std::string>& parts);
    std::string handle_pay(const std::vector<std::string>& parts);
    std::string handle_balance(const std::vector<std::string>& parts);
    std::string handle_history(const std::vector<std::string>& parts);
    std::string handle_get_tx_status(const std::vector<std::string>& parts);

    static std::string ok(const std::string& payload = "");
    static std::string err(const std::string& code);
    static std::vector<std::string> split(const std::string& s, char delim);
};

} // namespace psp
