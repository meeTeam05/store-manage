#include "domain/customer/customer.hpp"

namespace fashion_store::domain::customer {

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

}  // namespace fashion_store::domain::customer
