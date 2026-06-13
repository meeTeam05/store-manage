#pragma once

#include <string>

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

private:
    EmployeeId id_;
    AccountId account_id_;
    std::string full_name_;
    std::string position_;
};

class IEmployeeRepository {
public:
    virtual ~IEmployeeRepository() = default;
};

}  // namespace fashion_store::domain::staff
