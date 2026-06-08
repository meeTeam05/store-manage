#pragma once

#include <vector>

#include "domain/shared/types.hpp"

namespace fashion_store::domain::cart {

using fashion_store::domain::shared::CartId;
using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::Money;
using fashion_store::domain::shared::Status;
using fashion_store::domain::shared::VariantId;
using fashion_store::domain::shared::ok_status;
using fashion_store::domain::shared::require;

enum class CartError {
    InvalidQuantity,
    ItemNotFound
};

struct CartItem {
    VariantId variant_id;
    int quantity;
    Money unit_price_snapshot;
};

class Cart {
public:
    Cart(CartId id, CustomerId customer_id)
        : id_(std::move(id)), customer_id_(std::move(customer_id)) {
        require(!id_.value.empty(), "cart id must not be empty");
        require(!customer_id_.value.empty(), "customer id must not be empty");
    }

    const CartId& id() const noexcept { return id_; }
    const CustomerId& customer_id() const noexcept { return customer_id_; }
    const std::vector<CartItem>& items() const noexcept { return items_; }
    bool empty() const noexcept { return items_.empty(); }

    Status<CartError> add_item(const VariantId& variant_id, int quantity, Money unit_price_snapshot) {
        if (quantity <= 0) {
            return Status<CartError>::fail(CartError::InvalidQuantity);
        }
        for (auto& item : items_) {
            if (item.variant_id == variant_id) {
                item.quantity += quantity;
                item.unit_price_snapshot = unit_price_snapshot;
                return ok_status<CartError>();
            }
        }
        items_.push_back(CartItem{variant_id, quantity, unit_price_snapshot});
        return ok_status<CartError>();
    }

    void clear() noexcept {
        items_.clear();
    }

    Money subtotal() const noexcept {
        auto subtotal = Money::from_minor(0);
        for (const auto& item : items_) {
            subtotal = subtotal + item.unit_price_snapshot.multiply(item.quantity);
        }
        return subtotal;
    }

private:
    CartId id_;
    CustomerId customer_id_;
    std::vector<CartItem> items_;
};

}  // namespace fashion_store::domain::cart
