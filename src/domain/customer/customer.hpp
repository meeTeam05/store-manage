#pragma once

#include <string>
#include <vector>

#include "domain/shared/types.hpp"

namespace fashion_store::domain::customer {

using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::ShippingAddress;
using fashion_store::domain::shared::require;

class Wishlist {
public:
    void add(const ProductId& product_id) {
        for (const auto& saved : product_ids_) {
            if (saved == product_id) {
                return;
            }
        }
        product_ids_.push_back(product_id);
    }

private:
    std::vector<ProductId> product_ids_;
};

class Customer {
public:
    Customer(CustomerId id, std::string full_name, std::string phone, ShippingAddress default_shipping_address)
        : id_(std::move(id)),
          full_name_(std::move(full_name)),
          phone_(std::move(phone)),
          default_shipping_address_(std::move(default_shipping_address)) {
        require(!id_.value.empty(), "customer id must not be empty");
        require(!full_name_.empty(), "customer full name must not be empty");
    }

private:
    CustomerId id_;
    std::string full_name_;
    std::string phone_;
    ShippingAddress default_shipping_address_;
    Wishlist wishlist_;
};

class ICustomerRepository {
public:
    virtual ~ICustomerRepository() = default;
};

}  // namespace fashion_store::domain::customer
