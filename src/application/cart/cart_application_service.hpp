#pragma once

#include "domain/cart/cart_repository.hpp"
#include "domain/catalog/product_repository.hpp"
#include "domain/shared/types.hpp"

namespace fashion_store::application::cart {

using fashion_store::domain::cart::Cart;
using fashion_store::domain::cart::ICartRepository;
using fashion_store::domain::catalog::IProductRepository;
using fashion_store::domain::shared::CartId;
using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::Result;
using fashion_store::domain::shared::VariantId;

enum class CartServiceError {
    ProductVariantNotFound,
    CartRuleViolation
};

class CartApplicationService {
public:
    CartApplicationService(ICartRepository& cart_repository, IProductRepository& product_repository)
        : cart_repository_(cart_repository), product_repository_(product_repository) {}

    Result<Cart, CartServiceError> add_item(const CartId& cart_id,
                                            const CustomerId& customer_id,
                                            const VariantId& variant_id,
                                            int quantity) {
        auto variant_view = product_repository_.find_variant(variant_id);
        if (!variant_view.has_value()) {
            return Result<Cart, CartServiceError>::fail(CartServiceError::ProductVariantNotFound);
        }

        auto cart = cart_repository_.find_by_id(cart_id).value_or(Cart(cart_id, customer_id));
        auto add_result = cart.add_item(variant_id, quantity, variant_view->variant.price);
        if (!add_result) {
            return Result<Cart, CartServiceError>::fail(CartServiceError::CartRuleViolation);
        }

        cart_repository_.save(cart);
        return Result<Cart, CartServiceError>::ok(cart);
    }

    Result<Cart, CartServiceError> get_or_create_cart(const CartId& cart_id, const CustomerId& customer_id) {
        auto cart = cart_repository_.find_by_id(cart_id).value_or(Cart(cart_id, customer_id));
        cart_repository_.save(cart);
        return Result<Cart, CartServiceError>::ok(cart);
    }

    Result<Cart, CartServiceError> change_quantity(const CartId& cart_id,
                                                   const VariantId& variant_id,
                                                   int quantity) {
        auto cart = cart_repository_.find_by_id(cart_id);
        if (!cart.has_value()) {
            return Result<Cart, CartServiceError>::fail(CartServiceError::CartRuleViolation);
        }
        auto change_result = cart->change_quantity(variant_id, quantity);
        if (!change_result) {
            return Result<Cart, CartServiceError>::fail(CartServiceError::CartRuleViolation);
        }
        cart_repository_.save(*cart);
        return Result<Cart, CartServiceError>::ok(*cart);
    }

    Result<Cart, CartServiceError> remove_item(const CartId& cart_id, const VariantId& variant_id) {
        auto cart = cart_repository_.find_by_id(cart_id);
        if (!cart.has_value()) {
            return Result<Cart, CartServiceError>::fail(CartServiceError::CartRuleViolation);
        }
        auto remove_result = cart->remove_item(variant_id);
        if (!remove_result) {
            return Result<Cart, CartServiceError>::fail(CartServiceError::CartRuleViolation);
        }
        cart_repository_.save(*cart);
        return Result<Cart, CartServiceError>::ok(*cart);
    }

private:
    ICartRepository& cart_repository_;
    IProductRepository& product_repository_;
};

}  // namespace fashion_store::application::cart
