#include "domain/identity/account.hpp"

namespace fashion_store::domain::identity {

Account::Account(AccountId id, std::string username, std::string password_hash, Role role, AccountStatus status)
    : id_(std::move(id)),
      username_(std::move(username)),
      password_hash_(std::move(password_hash)),
      role_(role),
      status_(status) {
    require(!id_.value.empty(), "account id must not be empty");
    require(!username_.empty(), "username must not be empty");
}

bool Account::can_sign_in() const noexcept {
    return status_ == AccountStatus::Active;
}

bool Account::password_matches(const std::string& password_hash) const noexcept {
    return password_hash_ == password_hash;
}

void Account::lock() noexcept {
    status_ = AccountStatus::Locked;
}

void Account::activate() noexcept {
    status_ = AccountStatus::Active;
}

}  // namespace fashion_store::domain::identity
