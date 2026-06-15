#pragma once

#include <string>

#include "domain/identity/account.hpp"

namespace fashion_store::application::identity {

using fashion_store::domain::identity::Account;
using fashion_store::domain::identity::AccountStatus;
using fashion_store::domain::identity::IAccountRepository;
using fashion_store::domain::identity::Role;
using fashion_store::domain::shared::Result;

enum class SignInError {
    AccountNotFound,
    AccountLocked,
    InvalidCredentials
};

struct SignInResult {
    Account account;
    Role role;
};

class AuthApplicationService {
public:
    explicit AuthApplicationService(IAccountRepository& account_repository)
        : account_repository_(account_repository) {}

    Result<SignInResult, SignInError> sign_in(const std::string& username,
                                              const std::string& password_hash) const {
        auto account = account_repository_.find_by_username(username);
        if (!account.has_value()) {
            return Result<SignInResult, SignInError>::fail(SignInError::AccountNotFound);
        }
        if (!account->can_sign_in()) {
            return Result<SignInResult, SignInError>::fail(SignInError::AccountLocked);
        }
        if (!account->password_matches(password_hash)) {
            return Result<SignInResult, SignInError>::fail(SignInError::InvalidCredentials);
        }
        return Result<SignInResult, SignInError>::ok(SignInResult{*account, account->role()});
    }

private:
    IAccountRepository& account_repository_;
};

}  // namespace fashion_store::application::identity
