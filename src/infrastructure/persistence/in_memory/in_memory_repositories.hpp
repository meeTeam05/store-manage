#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "domain/cart/cart_repository.hpp"
#include "domain/catalog/product_repository.hpp"
#include "domain/inventory/inventory_repository.hpp"
#include "domain/order/order_repository.hpp"
#include "domain/pricing/voucher_repository.hpp"
#include "domain/review/review.hpp"
#include "domain/returns/return_request.hpp"

namespace fashion_store::infrastructure::persistence::in_memory {

using fashion_store::domain::cart::Cart;
using fashion_store::domain::cart::ICartRepository;
using fashion_store::domain::catalog::CatalogVariantView;
using fashion_store::domain::catalog::IProductRepository;
using fashion_store::domain::catalog::Product;
using fashion_store::domain::inventory::IInventoryRepository;
using fashion_store::domain::inventory::InventoryItem;
using fashion_store::domain::order::IOrderRepository;
using fashion_store::domain::order::Order;
using fashion_store::domain::pricing::IVoucherRepository;
using fashion_store::domain::pricing::Voucher;
using fashion_store::domain::review::IReviewRepository;
using fashion_store::domain::review::Review;
using fashion_store::domain::returns::IReturnRepository;
using fashion_store::domain::returns::ReturnRequest;
using fashion_store::domain::shared::CartId;
using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::OrderId;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::VariantId;

class InMemoryProductRepository : public IProductRepository {
public:
    std::optional<Product> find_by_id(const ProductId& product_id) const override {
        auto it = products_.find(product_id.value);
        if (it == products_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<CatalogVariantView> find_variant(const VariantId& variant_id) const override {
        for (const auto& [_, product] : products_) {
            const auto* variant = product.find_variant(variant_id);
            if (variant != nullptr) {
                return CatalogVariantView{product.id(), product.name(), *variant};
            }
        }
        return std::nullopt;
    }

    void save(const Product& product) override {
        products_.insert_or_assign(product.id().value, product);
    }

private:
    std::unordered_map<std::string, Product> products_;
};

class InMemoryInventoryRepository : public IInventoryRepository {
public:
    std::optional<InventoryItem> find_by_variant_id(const VariantId& variant_id) const override {
        auto it = items_.find(variant_id.value);
        if (it == items_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void save(const InventoryItem& item) override {
        items_.insert_or_assign(item.variant_id().value, item);
    }

private:
    std::unordered_map<std::string, InventoryItem> items_;
};

class InMemoryCartRepository : public ICartRepository {
public:
    std::optional<Cart> find_by_id(const CartId& cart_id) const override {
        auto it = carts_.find(cart_id.value);
        if (it == carts_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<Cart> find_by_customer_id(const CustomerId& customer_id) const override {
        for (const auto& [_, cart] : carts_) {
            if (cart.customer_id() == customer_id) {
                return cart;
            }
        }
        return std::nullopt;
    }

    void save(const Cart& cart) override {
        carts_.insert_or_assign(cart.id().value, cart);
    }

private:
    std::unordered_map<std::string, Cart> carts_;
};

class InMemoryOrderRepository : public IOrderRepository {
public:
    std::optional<Order> find_by_id(const OrderId& order_id) const override {
        auto it = orders_.find(order_id.value);
        if (it == orders_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void save(const Order& order) override {
        orders_.insert_or_assign(order.id().value, order);
    }

private:
    std::unordered_map<std::string, Order> orders_;
};

class InMemoryVoucherRepository : public IVoucherRepository {
public:
    std::optional<Voucher> find_by_code(std::string_view code) const override {
        auto it = vouchers_.find(std::string(code));
        if (it == vouchers_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void save(const Voucher& voucher) override {
        vouchers_.insert_or_assign(voucher.code(), voucher);
    }

private:
    std::unordered_map<std::string, Voucher> vouchers_;
};

class InMemoryReviewRepository : public IReviewRepository {
public:
    void save(const Review& review) override {
        reviews_.push_back(review);
    }

private:
    std::vector<Review> reviews_;
};

class InMemoryReturnRepository : public IReturnRepository {
public:
    void save(const ReturnRequest& request) override {
        requests_.push_back(request);
    }

private:
    std::vector<ReturnRequest> requests_;
};

}  // namespace fashion_store::infrastructure::persistence::in_memory
