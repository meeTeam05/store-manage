#include "domain/customer/customer.hpp"

namespace fashion_store::domain::customer {

void Wishlist::add(const ProductId& product_id) {
    for (const auto& saved : product_ids_) {
        if (saved == product_id) {
            return;
        }
    }
    product_ids_.push_back(product_id);
}

bool Wishlist::contains(const ProductId& product_id) const noexcept {
    for (const auto& saved : product_ids_) {
        if (saved == product_id) {
            return true;
        }
    }
    return false;
}

void Wishlist::remove(const ProductId& product_id) {
    for (auto it = product_ids_.begin(); it != product_ids_.end(); ++it) {
        if (*it == product_id) {
            product_ids_.erase(it);
            return;
        }
    }
}

Customer::Customer(CustomerId id,
                   AccountId account_id,
                   std::string full_name,
                   std::string phone,
                   ShippingAddress default_shipping_address)
    : id_(std::move(id)),
      account_id_(std::move(account_id)),
      full_name_(std::move(full_name)),
      phone_(std::move(phone)),
      default_shipping_address_(std::move(default_shipping_address)) {
    require(!id_.value.empty(), "customer id must not be empty");
    require(!account_id_.value.empty(), "customer account id must not be empty");
    require(!full_name_.empty(), "customer full name must not be empty");
}

void Customer::change_phone(std::string phone) {
    require(!phone.empty(), "customer phone must not be empty");
    phone_ = std::move(phone);
}

void Customer::update_default_shipping_address(ShippingAddress address) {
    default_shipping_address_ = std::move(address);
}

void Customer::add_to_wishlist(const ProductId& product_id) {
    wishlist_.add(product_id);
}

void Customer::remove_from_wishlist(const ProductId& product_id) {
    wishlist_.remove(product_id);
}

}  // namespace fashion_store::domain::customer
