#pragma once

#include <exception>
#include <optional>
#include <string>
#include <vector>

#include "application/order/order_application_service.hpp"
#include "domain/catalog/product_repository.hpp"
#include "domain/inventory/inventory_repository.hpp"
#include "domain/order/order_repository.hpp"
#include "domain/pricing/voucher_repository.hpp"

namespace fashion_store::application::staff {

using fashion_store::application::order::OrderApplicationService;
using fashion_store::application::order::PlaceOrderError;
using fashion_store::domain::catalog::Category;
using fashion_store::domain::catalog::IProductRepository;
using fashion_store::domain::catalog::Product;
using fashion_store::domain::catalog::ProductError;
using fashion_store::domain::catalog::ProductStatus;
using fashion_store::domain::catalog::ProductVariantDraft;
using fashion_store::domain::inventory::IInventoryRepository;
using fashion_store::domain::inventory::InventoryItem;
using fashion_store::domain::order::IOrderRepository;
using fashion_store::domain::order::Order;
using fashion_store::domain::order::OrderStatus;
using fashion_store::domain::pricing::IVoucherRepository;
using fashion_store::domain::pricing::Voucher;
using fashion_store::domain::shared::Money;
using fashion_store::domain::shared::OrderId;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::Result;
using fashion_store::domain::shared::VariantId;

enum class StoreManagementError {
    ProductNotFound,
    ProductAlreadyExists,
    ProductRuleViolation,
    InventoryNotFound,
    InvalidInventoryQuantity,
    OrderRuleViolation
};

struct ProductDraft {
    ProductId id;
    std::string name;
    Category category;
    std::string description;
    std::string collection;
    ProductStatus status{ProductStatus::Active};
};

class StoreManagementService {
public:
    StoreManagementService(IProductRepository& product_repository,
                           IInventoryRepository& inventory_repository,
                           IOrderRepository& order_repository,
                           IVoucherRepository& voucher_repository,
                           OrderApplicationService& order_service)
        : product_repository_(product_repository),
          inventory_repository_(inventory_repository),
          order_repository_(order_repository),
          voucher_repository_(voucher_repository),
          order_service_(order_service) {}

    Result<Product, StoreManagementError> create_product(const ProductDraft& draft) {
        if (product_repository_.find_by_id(draft.id).has_value()) {
            return Result<Product, StoreManagementError>::fail(StoreManagementError::ProductAlreadyExists);
        }

        try {
            auto product = Product::rehydrate(
                draft.id,
                draft.name,
                draft.category,
                draft.description,
                draft.collection,
                draft.status,
                {});
            product_repository_.save(product);
            return Result<Product, StoreManagementError>::ok(product);
        } catch (const std::exception&) {
            return Result<Product, StoreManagementError>::fail(StoreManagementError::ProductRuleViolation);
        }
    }

    Result<Product, StoreManagementError> update_product(const ProductDraft& draft) {
        auto product = product_repository_.find_by_id(draft.id);
        if (!product.has_value()) {
            return Result<Product, StoreManagementError>::fail(StoreManagementError::ProductNotFound);
        }

        try {
            auto updated_product = Product::rehydrate(
                draft.id,
                draft.name,
                draft.category,
                draft.description,
                draft.collection,
                draft.status,
                product->variants());
            product_repository_.save(updated_product);
            return Result<Product, StoreManagementError>::ok(updated_product);
        } catch (const std::exception&) {
            return Result<Product, StoreManagementError>::fail(StoreManagementError::ProductRuleViolation);
        }
    }

    Result<Product, StoreManagementError> update_product_status(const ProductId& product_id,
                                                                ProductStatus status) {
        auto product = product_repository_.find_by_id(product_id);
        if (!product.has_value()) {
            return Result<Product, StoreManagementError>::fail(StoreManagementError::ProductNotFound);
        }

        auto updated_product = Product::rehydrate(
            product->id(),
            product->name(),
            product->category(),
            product->description(),
            product->collection(),
            status,
            product->variants());
        product_repository_.save(updated_product);
        return Result<Product, StoreManagementError>::ok(updated_product);
    }

    Result<Product, StoreManagementError> add_product_variant(const ProductId& product_id,
                                                             const ProductVariantDraft& draft) {
        auto product = product_repository_.find_by_id(product_id);
        if (!product.has_value()) {
            return Result<Product, StoreManagementError>::fail(StoreManagementError::ProductNotFound);
        }

        auto add_result = product->add_variant(draft);
        if (!add_result) {
            return Result<Product, StoreManagementError>::fail(StoreManagementError::ProductRuleViolation);
        }

        product_repository_.save(*product);
        return Result<Product, StoreManagementError>::ok(*product);
    }

    Result<InventoryItem, StoreManagementError> set_inventory(const VariantId& variant_id,
                                                              int on_hand,
                                                              int reserved = 0) {
        if (on_hand < 0 || reserved < 0 || reserved > on_hand) {
            return Result<InventoryItem, StoreManagementError>::fail(StoreManagementError::InvalidInventoryQuantity);
        }

        InventoryItem item(variant_id, on_hand, reserved);
        inventory_repository_.save(item);
        return Result<InventoryItem, StoreManagementError>::ok(item);
    }

    Result<InventoryItem, StoreManagementError> restock_inventory(const VariantId& variant_id, int quantity) {
        auto item = inventory_repository_.find_by_variant_id(variant_id);
        if (!item.has_value()) {
            return Result<InventoryItem, StoreManagementError>::fail(StoreManagementError::InventoryNotFound);
        }

        auto restock_result = item->restock(quantity);
        if (!restock_result) {
            return Result<InventoryItem, StoreManagementError>::fail(StoreManagementError::InvalidInventoryQuantity);
        }

        inventory_repository_.save(*item);
        return Result<InventoryItem, StoreManagementError>::ok(*item);
    }

    void save_voucher(const Voucher& voucher) {
        voucher_repository_.save(voucher);
    }

    std::optional<Order> find_order(const OrderId& order_id) const {
        return order_repository_.find_by_id(order_id);
    }

    std::vector<Order> list_orders() const {
        return order_repository_.list_all();
    }

    Result<Order, StoreManagementError> advance_order_status(const OrderId& order_id,
                                                             OrderStatus target_status) {
        auto result = order_service_.advance_order_status(order_id, target_status);
        if (!result) {
            return Result<Order, StoreManagementError>::fail(StoreManagementError::OrderRuleViolation);
        }
        return Result<Order, StoreManagementError>::ok(result.value());
    }

    Result<Order, StoreManagementError> cancel_order(const OrderId& order_id) {
        auto result = order_service_.cancel_order(order_id);
        if (!result) {
            return Result<Order, StoreManagementError>::fail(StoreManagementError::OrderRuleViolation);
        }
        return Result<Order, StoreManagementError>::ok(result.value());
    }

private:
    IProductRepository& product_repository_;
    IInventoryRepository& inventory_repository_;
    IOrderRepository& order_repository_;
    IVoucherRepository& voucher_repository_;
    OrderApplicationService& order_service_;
};

}  // namespace fashion_store::application::staff
