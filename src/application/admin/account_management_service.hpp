#pragma once

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <vector>

#include "domain/customer/customer.hpp"
#include "domain/identity/account.hpp"
#include "domain/staff/employee.hpp"

namespace fashion_store::application::admin {

using fashion_store::domain::customer::ICustomerRepository;
using fashion_store::domain::identity::Account;
using fashion_store::domain::identity::AccountStatus;
using fashion_store::domain::identity::IAccountRepository;
using fashion_store::domain::identity::Role;
using fashion_store::domain::shared::AccountId;
using fashion_store::domain::shared::Result;
using fashion_store::domain::staff::Employee;
using fashion_store::domain::staff::IEmployeeRepository;
using fashion_store::domain::shared::EmployeeId;

enum class AccountManagementError {
    AccountNotFound,
    UsernameAlreadyExists,
    InvalidRole,
    InvalidUsername,
    InvalidPassword,
    InvalidFullName
};

struct ManagedAccountView {
    std::string account_id;
    std::string username;
    std::string role;
    std::string status;
    std::optional<std::string> customer_id;
    std::optional<std::string> employee_id;
    std::string display_name;
    std::optional<std::string> position;
};

class AccountManagementService {
public:
    AccountManagementService(IAccountRepository& account_repository,
                             ICustomerRepository& customer_repository,
                             IEmployeeRepository& employee_repository)
        : account_repository_(account_repository),
          customer_repository_(customer_repository),
          employee_repository_(employee_repository) {}

    std::vector<ManagedAccountView> list_accounts(std::optional<Role> role_filter = std::nullopt,
                                                  std::optional<AccountStatus> status_filter = std::nullopt) const {
        std::vector<ManagedAccountView> views;
        for (const auto& account : account_repository_.list_all()) {
            if (role_filter.has_value() && account.role() != *role_filter) {
                continue;
            }
            if (status_filter.has_value() && account.status() != *status_filter) {
                continue;
            }
            views.push_back(to_view(account));
        }
        std::sort(views.begin(), views.end(), [](const ManagedAccountView& left, const ManagedAccountView& right) {
            return left.username < right.username;
        });
        return views;
    }

    Result<ManagedAccountView, AccountManagementError> create_employee_account(Role role,
                                                                               const std::string& username,
                                                                               const std::string& password_hash,
                                                                               const std::string& full_name) {
        if (role != Role::Staff && role != Role::Admin && role != Role::Manager) {
            return Result<ManagedAccountView, AccountManagementError>::fail(AccountManagementError::InvalidRole);
        }
        const auto clean_username = trim(username);
        const auto clean_password_hash = trim(password_hash);
        const auto clean_full_name = trim(full_name);
        if (clean_username.empty()) {
            return Result<ManagedAccountView, AccountManagementError>::fail(AccountManagementError::InvalidUsername);
        }
        if (clean_password_hash.empty()) {
            return Result<ManagedAccountView, AccountManagementError>::fail(AccountManagementError::InvalidPassword);
        }
        if (clean_full_name.empty()) {
            return Result<ManagedAccountView, AccountManagementError>::fail(AccountManagementError::InvalidFullName);
        }
        if (account_repository_.find_by_username(clean_username).has_value()) {
            return Result<ManagedAccountView, AccountManagementError>::fail(AccountManagementError::UsernameAlreadyExists);
        }

        const auto slug = slugify(clean_username);
        std::string account_id = "account-" + slug;
        std::string employee_id = "employee-" + slug;
        int suffix = 1;
        while (account_repository_.find_by_id(AccountId{account_id}).has_value() ||
               employee_repository_.find_by_id(EmployeeId{employee_id}).has_value()) {
            ++suffix;
            account_id = "account-" + slug + "-" + std::to_string(suffix);
            employee_id = "employee-" + slug + "-" + std::to_string(suffix);
        }

        Account account(
            AccountId{account_id},
            clean_username,
            clean_password_hash,
            role,
            AccountStatus::Active);
        Employee employee(
            EmployeeId{employee_id},
            account.id(),
            clean_full_name,
            role == Role::Admin ? "Administrator" : (role == Role::Manager ? "Manager" : "Operations Staff"));

        account_repository_.save(account);
        employee_repository_.save(employee);
        return Result<ManagedAccountView, AccountManagementError>::ok(to_view(account));
    }

    Result<ManagedAccountView, AccountManagementError> set_account_status(const AccountId& account_id,
                                                                          AccountStatus status) {
        auto account = account_repository_.find_by_id(account_id);
        if (!account.has_value()) {
            return Result<ManagedAccountView, AccountManagementError>::fail(AccountManagementError::AccountNotFound);
        }
        if (status == AccountStatus::Locked) {
            account->lock();
        } else {
            account->activate();
        }
        account_repository_.save(*account);
        return Result<ManagedAccountView, AccountManagementError>::ok(to_view(*account));
    }

    Result<ManagedAccountView, AccountManagementError> reset_password(const AccountId& account_id,
                                                                      const std::string& password_hash) {
        const auto clean_password_hash = trim(password_hash);
        if (clean_password_hash.empty()) {
            return Result<ManagedAccountView, AccountManagementError>::fail(AccountManagementError::InvalidPassword);
        }
        auto account = account_repository_.find_by_id(account_id);
        if (!account.has_value()) {
            return Result<ManagedAccountView, AccountManagementError>::fail(AccountManagementError::AccountNotFound);
        }
        account->change_password(clean_password_hash);
        account_repository_.save(*account);
        return Result<ManagedAccountView, AccountManagementError>::ok(to_view(*account));
    }

private:
    static std::string trim(std::string value) {
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
            value.erase(value.begin());
        }
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
            value.pop_back();
        }
        return value;
    }

    static std::string slugify(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        std::string output;
        bool last_dash = false;
        for (unsigned char ch : value) {
            if (std::isalnum(ch)) {
                output.push_back(static_cast<char>(ch));
                last_dash = false;
            } else if (!last_dash) {
                output.push_back('-');
                last_dash = true;
            }
        }
        while (!output.empty() && output.front() == '-') {
            output.erase(output.begin());
        }
        while (!output.empty() && output.back() == '-') {
            output.pop_back();
        }
        return output.empty() ? "staff" : output;
    }

    static std::string role_to_string(Role role) {
        switch (role) {
            case Role::Customer: return "Customer";
            case Role::Staff: return "Staff";
            case Role::Manager: return "Manager";
            case Role::Admin: return "Admin";
        }
        return "Customer";
    }

    static std::string status_to_string(AccountStatus status) {
        return status == AccountStatus::Locked ? "Locked" : "Active";
    }

    ManagedAccountView to_view(const Account& account) const {
        auto customer = customer_repository_.find_by_account_id(account.id());
        auto employee = employee_repository_.find_by_account_id(account.id());
        return ManagedAccountView{
            account.id().value,
            account.username(),
            role_to_string(account.role()),
            status_to_string(account.status()),
            customer.has_value() ? std::optional<std::string>{customer->id().value} : std::nullopt,
            employee.has_value() ? std::optional<std::string>{employee->id().value} : std::nullopt,
            customer.has_value() ? customer->full_name() : (employee.has_value() ? employee->full_name() : account.username()),
            employee.has_value() ? std::optional<std::string>{employee->position()} : std::nullopt
        };
    }

    IAccountRepository& account_repository_;
    ICustomerRepository& customer_repository_;
    IEmployeeRepository& employee_repository_;
};

}  // namespace fashion_store::application::admin
