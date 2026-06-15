#pragma once

#include <optional>
#include <string>
#include <vector>

#include "domain/shared/types.hpp"

namespace fashion_store::domain::customer {

using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::AccountId;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::ShippingAddress;
using fashion_store::domain::shared::require;

class Wishlist {
public:
    void add(const ProductId& product_id);

    bool contains(const ProductId& product_id) const noexcept;

    void remove(const ProductId& product_id);

    const std::vector<ProductId>& items() const noexcept {
        return product_ids_;
    }

private:
    std::vector<ProductId> product_ids_;
};

class Customer {
public:
    Customer(CustomerId id,
             AccountId account_id,
             std::string full_name,
             std::string phone,
             ShippingAddress default_shipping_address);

    const CustomerId& id() const noexcept { return id_; }
    const AccountId& account_id() const noexcept { return account_id_; }
    const std::string& full_name() const noexcept { return full_name_; }
    const std::string& phone() const noexcept { return phone_; }
    const ShippingAddress& default_shipping_address() const noexcept { return default_shipping_address_; }
    const Wishlist& wishlist() const noexcept { return wishlist_; }

    void change_phone(std::string phone);

    void update_default_shipping_address(ShippingAddress address);

    void add_to_wishlist(const ProductId& product_id);

    void remove_from_wishlist(const ProductId& product_id);

private:
    CustomerId id_;
    AccountId account_id_;
    std::string full_name_;
    std::string phone_;
    ShippingAddress default_shipping_address_;
    Wishlist wishlist_;
};

class ICustomerRepository {
public:
    virtual ~ICustomerRepository() = default;

    virtual std::optional<Customer> find_by_id(const CustomerId& customer_id) const = 0;
    virtual std::optional<Customer> find_by_account_id(const AccountId& account_id) const = 0;
    virtual void save(const Customer& customer) = 0;
};

}  // namespace fashion_store::domain::customer
