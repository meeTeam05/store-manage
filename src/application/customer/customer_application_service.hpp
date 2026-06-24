#pragma once

#include <optional>
#include <string>

#include "domain/customer/customer.hpp"

namespace fashion_store::application::customer {

using fashion_store::domain::customer::Customer;
using fashion_store::domain::customer::ICustomerRepository;
using fashion_store::domain::shared::AccountId;
using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::Result;

struct CustomerProfileView {
    std::string full_name;
    std::string phone;
    std::string city;
};

enum class CustomerServiceError {
    CustomerNotFound
};

class CustomerApplicationService {
public:
    explicit CustomerApplicationService(ICustomerRepository& customer_repository)
        : customer_repository_(customer_repository) {}

    Result<CustomerProfileView, CustomerServiceError> get_profile(const CustomerId& customer_id) const {
        auto customer = customer_repository_.find_by_id(customer_id);
        if (!customer.has_value()) {
            return Result<CustomerProfileView, CustomerServiceError>::fail(CustomerServiceError::CustomerNotFound);
        }
        return Result<CustomerProfileView, CustomerServiceError>::ok(to_profile_view(*customer));
    }

    Result<CustomerProfileView, CustomerServiceError> get_profile_by_account_id(const AccountId& account_id) const {
        auto customer = customer_repository_.find_by_account_id(account_id);
        if (!customer.has_value()) {
            return Result<CustomerProfileView, CustomerServiceError>::fail(CustomerServiceError::CustomerNotFound);
        }
        return Result<CustomerProfileView, CustomerServiceError>::ok(to_profile_view(*customer));
    }



private:
    static CustomerProfileView to_profile_view(const Customer& customer) {
        return CustomerProfileView{
            customer.full_name(),
            customer.phone(),
            customer.default_shipping_address().city
        };
    }

    ICustomerRepository& customer_repository_;
};

}  // namespace fashion_store::application::customer
