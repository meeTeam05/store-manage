#pragma once

#include <optional>

#include "domain/cart/cart.hpp"

namespace fashion_store::domain::cart {

class ICartRepository {
public:
    virtual ~ICartRepository() = default;

    virtual std::optional<Cart> find_by_id(const CartId& cart_id) const = 0;
    virtual std::optional<Cart> find_by_customer_id(const CustomerId& customer_id) const = 0;
    virtual void save(const Cart& cart) = 0;
};

}  // namespace fashion_store::domain::cart
