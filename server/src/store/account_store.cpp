#include "store/account_store.hpp"

uint64_t AccountStore::create(const std::string& name, AccountType type) {
    const uint64_t id = next_id_.fetch_add(1);
    auto account = std::make_shared<Account>(id, name, type);
    std::unique_lock lock(mtx_);
    accounts_.emplace(id, std::move(account));
    return id;
}

std::shared_ptr<Account> AccountStore::find(uint64_t account_id) {
    std::shared_lock lock(mtx_);
    auto it = accounts_.find(account_id);
    if (it == accounts_.end()) {
        return nullptr;
    }
    return it->second;
}
