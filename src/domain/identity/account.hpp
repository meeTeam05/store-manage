#pragma once

#include <optional>
#include <string>
#include <vector>

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
    Account(AccountId id, std::string username, std::string password_hash, Role role, AccountStatus status);

    const AccountId& id() const noexcept { return id_; }
    const std::string& username() const noexcept { return username_; }
    const std::string& password_hash() const noexcept { return password_hash_; }
    Role role() const noexcept { return role_; }
    AccountStatus status() const noexcept { return status_; }

    bool can_sign_in() const noexcept;

    bool password_matches(const std::string& password_hash) const noexcept;

    void lock() noexcept;

    void activate() noexcept;

    void change_password(std::string password_hash);

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

    virtual std::optional<Account> find_by_id(const AccountId& account_id) const = 0;
    virtual std::optional<Account> find_by_username(const std::string& username) const = 0;
    virtual std::vector<Account> list_all() const = 0;
    virtual void save(const Account& account) = 0;
};

}  // namespace fashion_store::domain::identity
