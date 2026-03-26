#pragma once
#include <stdexcept>

struct PspError            : std::runtime_error { using std::runtime_error::runtime_error; };
struct AccountNotFound     : PspError { AccountNotFound()     : PspError("account_not_found") {} };
struct InvalidAmount       : PspError { InvalidAmount()       : PspError("invalid_amount") {} };
struct TransactionNotFound : PspError { TransactionNotFound() : PspError("transaction_not_found") {} };
struct InvalidAccountType  : PspError { InvalidAccountType()  : PspError("invalid_account_type") {} };
