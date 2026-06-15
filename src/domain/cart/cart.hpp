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
    Cart(CartId id, CustomerId customer_id);

    const CartId& id() const noexcept { return id_; }
    const CustomerId& customer_id() const noexcept { return customer_id_; }
    const std::vector<CartItem>& items() const noexcept { return items_; }
    bool empty() const noexcept { return items_.empty(); }

    Status<CartError> add_item(const VariantId& variant_id, int quantity, Money unit_price_snapshot);

    Status<CartError> change_quantity(const VariantId& variant_id, int quantity);

    Status<CartError> remove_item(const VariantId& variant_id);

    const CartItem* find_item(const VariantId& variant_id) const noexcept;

    void clear() noexcept {
        items_.clear();
    }

    Money subtotal() const noexcept;

private:
    CartId id_;
    CustomerId customer_id_;
    std::vector<CartItem> items_;
};

}  // namespace fashion_store::domain::cart
