#include "net/request_handler.hpp"

#include <sstream>
#include <chrono>
#include <ctime>

#include "error.hpp"

namespace psp {

RequestHandler::RequestHandler(PaymentService& service)
    : service_(service) {}

std::string RequestHandler::handle(const std::string& request) {
    try {
        auto parts = split(request, '|');
        if (parts.empty() || parts[0].empty()) {
            return err("invalid_request");
        }

        const auto& cmd = parts[0];
        if (cmd == "CREATE_ACCOUNT") return handle_create_account(parts);
        if (cmd == "DEPOSIT")        return handle_deposit(parts);
        if (cmd == "PAY")            return handle_pay(parts);
        if (cmd == "BALANCE")        return handle_balance(parts);
        if (cmd == "HISTORY")        return handle_history(parts);
        if (cmd == "GET_TX_STATUS")  return handle_get_tx_status(parts);

        return err("invalid_request");
    } catch (const PspError& e) {
        return err(e.what());
    } catch (const std::exception&) {
        return err("internal_error");
    }
}

std::string RequestHandler::handle_create_account(const std::vector<std::string>& parts) {
    if (parts.size() != 3) return err("invalid_request");

    const auto& name = parts[1];
    const auto& type_str = parts[2];

    AccountType type;
    if (type_str == "payer")        type = AccountType::Payer;
    else if (type_str == "merchant") type = AccountType::Merchant;
    else                             return err("invalid_request");

    const auto id = service_.create_account(name, type);
    return ok(std::to_string(id));
}

std::string RequestHandler::handle_deposit(const std::vector<std::string>& parts) {
    if (parts.size() != 3) return err("invalid_request");

    uint64_t account_id;
    int64_t  amount;
    try {
        account_id = std::stoull(parts[1]);
        amount     = std::stoll(parts[2]);
    } catch (...) {
        return err("invalid_request");
    }

    service_.deposit(account_id, amount);
    return ok();
}

std::string RequestHandler::handle_pay(const std::vector<std::string>& parts) {
    if (parts.size() != 4) return err("invalid_request");

    uint64_t from_id, to_id;
    int64_t  amount;
    try {
        from_id = std::stoull(parts[1]);
        to_id   = std::stoull(parts[2]);
        amount  = std::stoll(parts[3]);
    } catch (...) {
        return err("invalid_request");
    }

    auto [tx_id, fut] = service_.pay(from_id, to_id, amount);
    (void)fut;
    return ok("pending|" + std::to_string(tx_id));
}

std::string RequestHandler::handle_balance(const std::vector<std::string>& parts) {
    if (parts.size() != 2) return err("invalid_request");

    uint64_t account_id;
    try {
        account_id = std::stoull(parts[1]);
    } catch (...) {
        return err("invalid_request");
    }

    const auto bal = service_.balance(account_id);
    return ok(std::to_string(bal));
}

std::string RequestHandler::handle_history(const std::vector<std::string>& parts) {
    if (parts.size() != 2) return err("invalid_request");

    uint64_t account_id;
    try {
        account_id = std::stoull(parts[1]);
    } catch (...) {
        return err("invalid_request");
    }

    const auto txs = service_.history(account_id);

    std::string payload;
    for (std::size_t i = 0; i < txs.size(); ++i) {
        const auto& tx = txs[i];
        if (i > 0) payload += ";";

        const auto ts = std::chrono::system_clock::to_time_t(tx.created_at);
        std::string status_str;
        if (tx.status == TransactionStatus::Completed)   status_str = "completed";
        else if (tx.status == TransactionStatus::Failed) status_str = "failed";
        else                                              status_str = "pending";

        payload += std::to_string(tx.transaction_id)
            + "|" + std::to_string(tx.from_account_id)
            + "|" + std::to_string(tx.to_account_id)
            + "|" + std::to_string(tx.amount)
            + "|" + status_str
            + "|" + tx.fail_reason
            + "|" + std::to_string(ts);
    }

    return ok(payload);
}

std::string RequestHandler::handle_get_tx_status(const std::vector<std::string>& parts) {
    if (parts.size() != 2) return err("invalid_request");

    uint64_t tx_id;
    try {
        tx_id = std::stoull(parts[1]);
    } catch (...) {
        return err("invalid_request");
    }

    const auto tx = service_.get_transaction(tx_id);

    if (tx.status == TransactionStatus::Pending) {
        return ok("pending|" + std::to_string(tx_id));
    } else if (tx.status == TransactionStatus::Completed) {
        return ok("completed|" + std::to_string(tx_id));
    } else {
        return ok("failed|" + tx.fail_reason + "|" + std::to_string(tx_id));
    }
}

std::string RequestHandler::ok(const std::string& payload) {
    if (payload.empty()) return "OK\n";
    return "OK|" + payload + "\n";
}

std::string RequestHandler::err(const std::string& code) {
    return "ERR|" + code + "\n";
}

std::vector<std::string> RequestHandler::split(const std::string& s, char delim) {
    std::vector<std::string> result;
    std::istringstream stream(s);
    std::string token;
    while (std::getline(stream, token, delim)) {
        result.push_back(token);
    }
    return result;
}

} // namespace psp
