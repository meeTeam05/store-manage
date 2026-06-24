#pragma once

#include <optional>
#include <string>
#include <vector>

#include "domain/shared/types.hpp"

namespace fashion_store::domain::staff {

using fashion_store::domain::shared::AccountId;
using fashion_store::domain::shared::EmployeeId;
using fashion_store::domain::shared::require;

class Employee {
public:
    Employee(EmployeeId id, AccountId account_id, std::string full_name, std::string position)
        : id_(std::move(id)),
          account_id_(std::move(account_id)),
          full_name_(std::move(full_name)),
          position_(std::move(position)) {
        require(!id_.value.empty(), "employee id must not be empty");
        require(!account_id_.value.empty(), "employee account id must not be empty");
    }

    const EmployeeId& id() const noexcept { return id_; }
    const AccountId& account_id() const noexcept { return account_id_; }
    const std::string& full_name() const noexcept { return full_name_; }
    const std::string& position() const noexcept { return position_; }

private:
    EmployeeId id_;
    AccountId account_id_;
    std::string full_name_;
    std::string position_;
};

class IEmployeeRepository {
public:
    virtual ~IEmployeeRepository() = default;

    virtual std::optional<Employee> find_by_id(const EmployeeId& employee_id) const = 0;
    virtual std::optional<Employee> find_by_account_id(const AccountId& account_id) const = 0;
    virtual std::vector<Employee> list_all() const = 0;
    virtual void save(const Employee& employee) = 0;
};

}  // namespace fashion_store::domain::staff
