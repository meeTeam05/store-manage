#pragma once

#include <string>

#include "domain/shared/types.hpp"

namespace fashion_store::domain::identity {

using fashion_store::domain::shared::AccountId;
using fashion_store::domain::shared::require;

enum class Role {
    Customer,
    Staff,
    Manager,
    Admin
};

enum class AccountStatus {
    Active,
    Locked
};

class Account {
public:
    Account(AccountId id, std::string username, std::string password_hash, Role role, AccountStatus status)
        : id_(std::move(id)),
          username_(std::move(username)),
          password_hash_(std::move(password_hash)),
          role_(role),
          status_(status) {
        require(!id_.value.empty(), "account id must not be empty");
        require(!username_.empty(), "username must not be empty");
    }

private:
    AccountId id_;
    std::string username_;
    std::string password_hash_;
    Role role_;
    AccountStatus status_;
};

class IAccountRepository {
public:
    virtual ~IAccountRepository() = default;
};

}  // namespace fashion_store::domain::identity
