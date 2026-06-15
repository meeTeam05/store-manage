#include "domain/cart/cart.hpp"

namespace fashion_store::domain::cart {

Cart::Cart(CartId id, CustomerId customer_id)
    : id_(std::move(id)), customer_id_(std::move(customer_id)) {
    require(!id_.value.empty(), "cart id must not be empty");
    require(!customer_id_.value.empty(), "customer id must not be empty");
}

Status<CartError> Cart::add_item(const VariantId& variant_id, int quantity, Money unit_price_snapshot) {
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

Status<CartError> Cart::change_quantity(const VariantId& variant_id, int quantity) {
    if (quantity <= 0) {
        return Status<CartError>::fail(CartError::InvalidQuantity);
    }
    for (auto& item : items_) {
        if (item.variant_id == variant_id) {
            item.quantity = quantity;
            return ok_status<CartError>();
        }
    }
    return Status<CartError>::fail(CartError::ItemNotFound);
}

Status<CartError> Cart::remove_item(const VariantId& variant_id) {
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        if (it->variant_id == variant_id) {
            items_.erase(it);
            return ok_status<CartError>();
        }
    }
    return Status<CartError>::fail(CartError::ItemNotFound);
}

const CartItem* Cart::find_item(const VariantId& variant_id) const noexcept {
    for (const auto& item : items_) {
        if (item.variant_id == variant_id) {
            return &item;
        }
    }
    return nullptr;
}

Money Cart::subtotal() const noexcept {
    auto subtotal = Money::from_minor(0);
    for (const auto& item : items_) {
        subtotal = subtotal + item.unit_price_snapshot.multiply(item.quantity);
    }
    return subtotal;
}

}  // namespace fashion_store::domain::cart
