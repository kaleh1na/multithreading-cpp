#include <gtest/gtest.h>
#include <chrono>
#include <thread>

#include "store/account_store.hpp"
#include "store/transaction_store.hpp"
#include "service/payment_service.hpp"
#include "net/request_handler.hpp"

class HandlerTest : public ::testing::Test {
protected:
    AccountStore     accounts;
    TransactionStore transactions;
    PaymentService   service{accounts, transactions};
    psp::RequestHandler handler{service};
};

static std::string strip_newline(const std::string& s) {
    if (!s.empty() && s.back() == '\n') return s.substr(0, s.size() - 1);
    return s;
}

static std::string wait_for_tx_status(psp::RequestHandler& handler, const std::string& tx_id,
                                       const std::string& expected_prefix) {
    using namespace std::chrono_literals;
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto resp = strip_newline(handler.handle("GET_TX_STATUS|" + tx_id));
        if (resp.find(expected_prefix) != std::string::npos) {
            return resp;
        }
        std::this_thread::sleep_for(1ms);
    }
    return strip_newline(handler.handle("GET_TX_STATUS|" + tx_id));
}

TEST_F(HandlerTest, CreateAccountPayer) {
    const auto resp = strip_newline(handler.handle("CREATE_ACCOUNT|Alice|payer"));
    EXPECT_EQ(resp.substr(0, 3), "OK|");
}

TEST_F(HandlerTest, CreateAccountMerchant) {
    const auto resp = strip_newline(handler.handle("CREATE_ACCOUNT|Bob|merchant"));
    EXPECT_EQ(resp.substr(0, 3), "OK|");
}

TEST_F(HandlerTest, DepositOk) {
    handler.handle("CREATE_ACCOUNT|Alice|payer");
    const auto resp = strip_newline(handler.handle("DEPOSIT|1|10000"));
    EXPECT_EQ(resp, "OK");
}

TEST_F(HandlerTest, DepositInvalidAmount) {
    handler.handle("CREATE_ACCOUNT|Alice|payer");
    const auto resp = strip_newline(handler.handle("DEPOSIT|1|-100"));
    EXPECT_EQ(resp, "ERR|invalid_amount");
}

TEST_F(HandlerTest, DepositAccountNotFound) {
    const auto resp = strip_newline(handler.handle("DEPOSIT|999|100"));
    EXPECT_EQ(resp, "ERR|account_not_found");
}

TEST_F(HandlerTest, PayReturnsPending) {
    handler.handle("CREATE_ACCOUNT|Alice|payer");
    handler.handle("CREATE_ACCOUNT|Bob|merchant");
    handler.handle("DEPOSIT|1|10000");
    const auto resp = strip_newline(handler.handle("PAY|1|2|5000"));
    EXPECT_EQ(resp.substr(0, 10), "OK|pending");
}

TEST_F(HandlerTest, PayCompleted) {
    handler.handle("CREATE_ACCOUNT|Alice|payer");
    handler.handle("CREATE_ACCOUNT|Bob|merchant");
    handler.handle("DEPOSIT|1|10000");
    const auto pay_resp = strip_newline(handler.handle("PAY|1|2|5000"));
    ASSERT_EQ(pay_resp.substr(0, 10), "OK|pending");

    const std::string tx_id = pay_resp.substr(11);
    const auto status_resp = wait_for_tx_status(handler, tx_id, "OK|completed");
    EXPECT_NE(status_resp.find("OK|completed"), std::string::npos);
}

TEST_F(HandlerTest, PayInsufficientFunds) {
    handler.handle("CREATE_ACCOUNT|Alice|payer");
    handler.handle("CREATE_ACCOUNT|Bob|merchant");
    handler.handle("DEPOSIT|1|10000");
    const auto pay_resp = strip_newline(handler.handle("PAY|1|2|99999"));
    ASSERT_EQ(pay_resp.substr(0, 10), "OK|pending");

    const std::string tx_id = pay_resp.substr(11);
    const auto status_resp = wait_for_tx_status(handler, tx_id, "OK|failed");
    EXPECT_NE(status_resp.find("OK|failed|insufficient_funds"), std::string::npos);
}

TEST_F(HandlerTest, PayAccountNotFound) {
    handler.handle("CREATE_ACCOUNT|Alice|payer");
    handler.handle("DEPOSIT|1|10000");
    const auto resp = strip_newline(handler.handle("PAY|1|999|100"));
    EXPECT_EQ(resp, "ERR|account_not_found");
}

TEST_F(HandlerTest, GetTxStatusPending) {
    handler.handle("CREATE_ACCOUNT|Alice|payer");
    handler.handle("CREATE_ACCOUNT|Bob|merchant");
    handler.handle("DEPOSIT|1|10000");
    const auto pay_resp = strip_newline(handler.handle("PAY|1|2|5000"));
    ASSERT_EQ(pay_resp.substr(0, 10), "OK|pending");

    const std::string tx_id = pay_resp.substr(11);
    const auto status_resp = strip_newline(handler.handle("GET_TX_STATUS|" + tx_id));
    EXPECT_EQ(status_resp.substr(0, 3), "OK|");
}

TEST_F(HandlerTest, GetTxStatusNotFound) {
    const auto resp = strip_newline(handler.handle("GET_TX_STATUS|9999"));
    EXPECT_EQ(resp, "ERR|transaction_not_found");
}

TEST_F(HandlerTest, GetTxStatusCompleted) {
    handler.handle("CREATE_ACCOUNT|Alice|payer");
    handler.handle("CREATE_ACCOUNT|Bob|merchant");
    handler.handle("DEPOSIT|1|10000");
    const auto pay_resp = strip_newline(handler.handle("PAY|1|2|5000"));
    ASSERT_EQ(pay_resp.substr(0, 10), "OK|pending");

    const std::string tx_id = pay_resp.substr(11);
    const auto status_resp = wait_for_tx_status(handler, tx_id, "OK|completed");
    EXPECT_EQ(status_resp, "OK|completed|" + tx_id);
}

TEST_F(HandlerTest, GetTxStatusFailed) {
    handler.handle("CREATE_ACCOUNT|Alice|payer");
    handler.handle("CREATE_ACCOUNT|Bob|merchant");
    handler.handle("DEPOSIT|1|100");
    const auto pay_resp = strip_newline(handler.handle("PAY|1|2|99999"));
    ASSERT_EQ(pay_resp.substr(0, 10), "OK|pending");

    const std::string tx_id = pay_resp.substr(11);
    const auto status_resp = wait_for_tx_status(handler, tx_id, "OK|failed");
    EXPECT_EQ(status_resp, "OK|failed|insufficient_funds|" + tx_id);
}

TEST_F(HandlerTest, BalanceOk) {
    handler.handle("CREATE_ACCOUNT|Alice|payer");
    handler.handle("CREATE_ACCOUNT|Bob|merchant");
    handler.handle("DEPOSIT|1|10000");
    const auto pay_resp = strip_newline(handler.handle("PAY|1|2|5000"));
    ASSERT_EQ(pay_resp.substr(0, 10), "OK|pending");

    const std::string tx_id = pay_resp.substr(11);
    wait_for_tx_status(handler, tx_id, "OK|completed");

    const auto resp = strip_newline(handler.handle("BALANCE|1"));
    EXPECT_EQ(resp, "OK|5000");
}

TEST_F(HandlerTest, BalanceAccountNotFound) {
    const auto resp = strip_newline(handler.handle("BALANCE|999"));
    EXPECT_EQ(resp, "ERR|account_not_found");
}

TEST_F(HandlerTest, HistoryOk) {
    handler.handle("CREATE_ACCOUNT|Alice|payer");
    handler.handle("CREATE_ACCOUNT|Bob|merchant");
    handler.handle("DEPOSIT|1|10000");
    const auto pay_resp = strip_newline(handler.handle("PAY|1|2|5000"));
    ASSERT_EQ(pay_resp.substr(0, 10), "OK|pending");

    const std::string tx_id = pay_resp.substr(11);
    wait_for_tx_status(handler, tx_id, "OK|completed");

    const auto resp = strip_newline(handler.handle("HISTORY|1"));
    EXPECT_EQ(resp.substr(0, 3), "OK|");
}

TEST_F(HandlerTest, UnknownCommand) {
    const auto resp = strip_newline(handler.handle("UNKNOWN|foo"));
    EXPECT_EQ(resp, "ERR|invalid_request");
}

TEST_F(HandlerTest, EmptyRequest) {
    const auto resp = strip_newline(handler.handle(""));
    EXPECT_EQ(resp, "ERR|invalid_request");
}

TEST_F(HandlerTest, PayInvalidAmount) {
    handler.handle("CREATE_ACCOUNT|Alice|payer");
    handler.handle("CREATE_ACCOUNT|Bob|merchant");
    const auto resp = strip_newline(handler.handle("PAY|1|2|abc"));
    EXPECT_EQ(resp, "ERR|invalid_request");
}

TEST_F(HandlerTest, DepositToMerchantReturnsError) {
    handler.handle("CREATE_ACCOUNT|Shop|merchant");
    const auto resp = strip_newline(handler.handle("DEPOSIT|1|1000"));
    EXPECT_EQ(resp, "ERR|invalid_account_type");
}

TEST_F(HandlerTest, PayFromMerchantReturnsError) {
    handler.handle("CREATE_ACCOUNT|Shop|merchant");
    handler.handle("CREATE_ACCOUNT|Alice|payer");
    const auto resp = strip_newline(handler.handle("PAY|1|2|100"));
    EXPECT_EQ(resp, "ERR|invalid_account_type");
}
