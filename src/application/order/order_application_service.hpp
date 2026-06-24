#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "domain/cart/cart_repository.hpp"
#include "domain/catalog/product_repository.hpp"
#include "domain/inventory/inventory_repository.hpp"
#include "domain/order/order_repository.hpp"
#include "domain/pricing/voucher_repository.hpp"

namespace fashion_store::application::order {

using fashion_store::domain::cart::Cart;
using fashion_store::domain::cart::ICartRepository;
using fashion_store::domain::catalog::CatalogVariantView;
using fashion_store::domain::catalog::IProductRepository;
using fashion_store::domain::inventory::IInventoryRepository;
using fashion_store::domain::inventory::InventoryItem;
using fashion_store::domain::order::IOrderRepository;
using fashion_store::domain::order::Order;
using fashion_store::domain::order::OrderError;
using fashion_store::domain::order::OrderStatus;
using fashion_store::domain::order::OrderItem;
using fashion_store::domain::order::PaymentMethod;
using fashion_store::domain::pricing::IVoucherRepository;
using fashion_store::domain::pricing::Voucher;
using fashion_store::domain::shared::CartId;
using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::Money;
using fashion_store::domain::shared::OrderId;
using fashion_store::domain::shared::OrderItemId;
using fashion_store::domain::shared::Result;
using fashion_store::domain::shared::ShippingAddress;
using fashion_store::domain::shared::Status;
using fashion_store::domain::shared::VariantId;
using fashion_store::domain::shared::ok_status;

enum class PlaceOrderError {
    CartNotFound,
    EmptyCart,
    ProductVariantNotFound,
    InventoryNotFound,
    InsufficientStock,
    VoucherNotFound,
    VoucherInvalid,
    OrderRuleViolation
};

struct PlaceOrderCommand {
    OrderId order_id;
    CartId cart_id;
    CustomerId customer_id;
    ShippingAddress shipping_address;
    PaymentMethod payment_method;
    std::optional<std::string> voucher_code;
};

struct PlaceOrderReceipt {
    Order order;
    Money subtotal;
    Money discount;
    Money total;
};

struct CheckoutPreview {
    std::size_t item_count;
    Money subtotal;
    Money discount;
    Money total;
    bool voucher_applied;
};

struct VoucherEvaluation {
    Money discount{Money::from_minor(0)};
    std::optional<Voucher> staged_voucher;
    bool voucher_applied{false};
};

struct StagedOrderDraft {
    std::vector<OrderItem> order_items;
    std::vector<InventoryItem> staged_inventory_items;
};

struct CancellationPlan {
    std::vector<InventoryItem> staged_inventory_items;
    std::optional<Voucher> restored_voucher;
};

class OrderApplicationService {
public:
    OrderApplicationService(ICartRepository& cart_repository,
                            IOrderRepository& order_repository,
                            IProductRepository& product_repository,
                            IInventoryRepository& inventory_repository,
                            IVoucherRepository& voucher_repository)
        : cart_repository_(cart_repository),
          order_repository_(order_repository),
          product_repository_(product_repository),
          inventory_repository_(inventory_repository),
          voucher_repository_(voucher_repository) {}

    Result<CheckoutPreview, PlaceOrderError> preview_checkout(const CartId& cart_id,
                                                              std::optional<std::string> voucher_code) {
        auto cart = load_cart_for_checkout(cart_id);
        if (!cart) {
            return Result<CheckoutPreview, PlaceOrderError>::fail(cart.error());
        }
        auto availability = validate_cart_items(cart.value());
        if (!availability) {
            return Result<CheckoutPreview, PlaceOrderError>::fail(availability.error());
        }

        const auto subtotal = cart.value().subtotal();
        auto voucher = evaluate_voucher(subtotal, voucher_code, false);
        if (!voucher) {
            return Result<CheckoutPreview, PlaceOrderError>::fail(voucher.error());
        }

        return Result<CheckoutPreview, PlaceOrderError>::ok(CheckoutPreview{
            cart.value().items().size(),
            subtotal,
            voucher.value().discount,
            subtotal - voucher.value().discount,
            voucher.value().voucher_applied});
    }

    Result<PlaceOrderReceipt, PlaceOrderError> place_order(const PlaceOrderCommand& command) {
        auto cart = load_cart_for_checkout(command.cart_id);
        if (!cart) {
            return Result<PlaceOrderReceipt, PlaceOrderError>::fail(cart.error());
        }
        auto staged_order = stage_order_draft(cart.value(), command.order_id);
        if (!staged_order) {
            return Result<PlaceOrderReceipt, PlaceOrderError>::fail(staged_order.error());
        }

        Order order(command.order_id,
                    command.customer_id,
                    staged_order.value().order_items,
                    command.shipping_address,
                    command.payment_method);

        const auto subtotal = order.subtotal();
        auto voucher = evaluate_voucher(subtotal, command.voucher_code, true);
        if (!voucher) {
            return Result<PlaceOrderReceipt, PlaceOrderError>::fail(voucher.error());
        }

        auto discount_result = order.apply_discount(voucher.value().discount, command.voucher_code);
        if (!discount_result) {
            return Result<PlaceOrderReceipt, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }

        auto submit_result = order.submit_for_payment();
        if (!submit_result) {
            return Result<PlaceOrderReceipt, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }

        auto cleared_cart = cart.value();
        cleared_cart.clear();

        persist_inventory_items(staged_order.value().staged_inventory_items);
        persist_voucher(voucher.value().staged_voucher);
        order_repository_.save(order);
        cart_repository_.save(cleared_cart);

        return Result<PlaceOrderReceipt, PlaceOrderError>::ok(
            PlaceOrderReceipt{order, subtotal, voucher.value().discount, order.total()});
    }

    Result<Order, PlaceOrderError> mark_order_paid(const OrderId& order_id) {
        auto order = load_order(order_id);
        if (!order) {
            return Result<Order, PlaceOrderError>::fail(order.error());
        }
        auto staged_inventory_items = stage_paid_inventory_updates(order.value());
        if (!staged_inventory_items) {
            return Result<Order, PlaceOrderError>::fail(staged_inventory_items.error());
        }

        auto paid_order = order.value();
        auto paid_result = paid_order.mark_paid();
        if (!paid_result) {
            return Result<Order, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }

        persist_inventory_items(staged_inventory_items.value());
        order_repository_.save(paid_order);
        return Result<Order, PlaceOrderError>::ok(paid_order);
    }

    Result<Order, PlaceOrderError> advance_order_status(const OrderId& order_id, OrderStatus target_status) {
        auto order = load_order(order_id);
        if (!order) {
            return Result<Order, PlaceOrderError>::fail(order.error());
        }
        auto result = apply_order_status_transition(order.value(), target_status);
        if (!result) {
            return Result<Order, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }

        order_repository_.save(order.value());
        return Result<Order, PlaceOrderError>::ok(order.value());
    }

    Result<Order, PlaceOrderError> cancel_order(const OrderId& order_id) {
        auto order = load_order(order_id);
        if (!order) {
            return Result<Order, PlaceOrderError>::fail(order.error());
        }
        return cancel_loaded_order(order.value());
    }

    Result<Order, PlaceOrderError> cancel_customer_order(const CustomerId& customer_id, const OrderId& order_id) {
        auto order = load_order(order_id);
        if (!order) {
            return Result<Order, PlaceOrderError>::fail(order.error());
        }
        if (order.value().customer_id() != customer_id) {
            return Result<Order, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }
        return cancel_loaded_order(order.value());
    }

    Result<Order, PlaceOrderError> get_order(const OrderId& order_id) const {
        auto order = load_order(order_id);
        if (!order) {
            return Result<Order, PlaceOrderError>::fail(order.error());
        }
        return Result<Order, PlaceOrderError>::ok(order.value());
    }

    std::vector<Order> get_customer_orders(const CustomerId& customer_id) const {
        return order_repository_.find_by_customer_id(customer_id);
    }

private:
    Result<Order, PlaceOrderError> cancel_loaded_order(const Order& order) {
        auto cancellation_plan = stage_cancellation(order);
        if (!cancellation_plan) {
            return Result<Order, PlaceOrderError>::fail(cancellation_plan.error());
        }

        auto cancelled_order = order;
        auto cancel_result = cancelled_order.cancel();
        if (!cancel_result) {
            return Result<Order, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }

        persist_inventory_items(cancellation_plan.value().staged_inventory_items);
        persist_voucher(cancellation_plan.value().restored_voucher);
        order_repository_.save(cancelled_order);
        return Result<Order, PlaceOrderError>::ok(cancelled_order);
    }
    Result<Cart, PlaceOrderError> load_cart_for_checkout(const CartId& cart_id) const {
        auto cart = cart_repository_.find_by_id(cart_id);
        if (!cart.has_value()) {
            return Result<Cart, PlaceOrderError>::fail(PlaceOrderError::CartNotFound);
        }
        if (cart->empty()) {
            return Result<Cart, PlaceOrderError>::fail(PlaceOrderError::EmptyCart);
        }
        return Result<Cart, PlaceOrderError>::ok(*cart);
    }

    Result<Order, PlaceOrderError> load_order(const OrderId& order_id) const {
        auto order = order_repository_.find_by_id(order_id);
        if (!order.has_value()) {
            return Result<Order, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }
        return Result<Order, PlaceOrderError>::ok(*order);
    }

    Result<CatalogVariantView, PlaceOrderError> load_variant(const VariantId& variant_id) const {
        auto variant = product_repository_.find_variant(variant_id);
        if (!variant.has_value()) {
            return Result<CatalogVariantView, PlaceOrderError>::fail(PlaceOrderError::ProductVariantNotFound);
        }
        return Result<CatalogVariantView, PlaceOrderError>::ok(*variant);
    }

    Result<InventoryItem, PlaceOrderError> load_inventory_item(const VariantId& variant_id) const {
        auto inventory_item = inventory_repository_.find_by_variant_id(variant_id);
        if (!inventory_item.has_value()) {
            return Result<InventoryItem, PlaceOrderError>::fail(PlaceOrderError::InventoryNotFound);
        }
        return Result<InventoryItem, PlaceOrderError>::ok(*inventory_item);
    }

    Status<PlaceOrderError> validate_cart_items(const Cart& cart) const {
        for (const auto& item : cart.items()) {
            auto variant = load_variant(item.variant_id);
            if (!variant) {
                return Status<PlaceOrderError>::fail(variant.error());
            }
            auto inventory_item = load_inventory_item(item.variant_id);
            if (!inventory_item) {
                return Status<PlaceOrderError>::fail(inventory_item.error());
            }
            if (inventory_item.value().available() < item.quantity) {
                return Status<PlaceOrderError>::fail(PlaceOrderError::InsufficientStock);
            }
        }
        return ok_status<PlaceOrderError>();
    }

    Result<VoucherEvaluation, PlaceOrderError> evaluate_voucher(
        Money subtotal,
        const std::optional<std::string>& voucher_code,
        bool consume_voucher) {
        VoucherEvaluation evaluation;
        if (!voucher_code.has_value()) {
            return Result<VoucherEvaluation, PlaceOrderError>::ok(evaluation);
        }

        auto voucher = voucher_repository_.find_by_code(*voucher_code);
        if (!voucher.has_value()) {
            return Result<VoucherEvaluation, PlaceOrderError>::fail(PlaceOrderError::VoucherNotFound);
        }

        auto voucher_validation = voucher->validate(subtotal, std::chrono::system_clock::now());
        if (!voucher_validation) {
            return Result<VoucherEvaluation, PlaceOrderError>::fail(PlaceOrderError::VoucherInvalid);
        }

        evaluation.discount = voucher->calculate_discount(subtotal);
        evaluation.voucher_applied = true;
        if (consume_voucher) {
            auto consume_result = voucher->consume();
            if (!consume_result) {
                return Result<VoucherEvaluation, PlaceOrderError>::fail(PlaceOrderError::VoucherInvalid);
            }
            evaluation.staged_voucher = *voucher;
        }
        return Result<VoucherEvaluation, PlaceOrderError>::ok(evaluation);
    }

    Result<StagedOrderDraft, PlaceOrderError> stage_order_draft(const Cart& cart, const OrderId& order_id) const {
        StagedOrderDraft staged;
        for (const auto& item : cart.items()) {
            auto variant = load_variant(item.variant_id);
            if (!variant) {
                return Result<StagedOrderDraft, PlaceOrderError>::fail(variant.error());
            }

            auto inventory_item = load_inventory_item(item.variant_id);
            if (!inventory_item) {
                return Result<StagedOrderDraft, PlaceOrderError>::fail(inventory_item.error());
            }

            auto reserve_result = inventory_item.value().reserve(item.quantity);
            if (!reserve_result) {
                return Result<StagedOrderDraft, PlaceOrderError>::fail(PlaceOrderError::InsufficientStock);
            }
            staged.staged_inventory_items.push_back(inventory_item.value());
            staged.order_items.push_back(OrderItem{
                OrderItemId{order_id.value + "-" + item.variant_id.value},
                variant.value().product_id,
                variant.value().variant.id,
                variant.value().product_name,
                variant.value().variant.sku,
                variant.value().variant.size,
                variant.value().variant.color,
                variant.value().variant.price,
                item.quantity,
                Money::from_minor(0)
            });
        }
        return Result<StagedOrderDraft, PlaceOrderError>::ok(staged);
    }

    Result<std::vector<InventoryItem>, PlaceOrderError> stage_paid_inventory_updates(const Order& order) const {
        std::vector<InventoryItem> staged_inventory_items;
        for (const auto& item : order.items()) {
            auto inventory_item = load_inventory_item(item.variant_id);
            if (!inventory_item) {
                return Result<std::vector<InventoryItem>, PlaceOrderError>::fail(inventory_item.error());
            }
            auto commit_result = inventory_item.value().commit_sale(item.quantity);
            if (!commit_result) {
                return Result<std::vector<InventoryItem>, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
            }
            staged_inventory_items.push_back(inventory_item.value());
        }
        return Result<std::vector<InventoryItem>, PlaceOrderError>::ok(staged_inventory_items);
    }

    Result<CancellationPlan, PlaceOrderError> stage_cancellation(const Order& order) {
        CancellationPlan plan;
        const auto status_before_cancel = order.status();

        if (status_before_cancel == OrderStatus::Draft ||
            status_before_cancel == OrderStatus::AwaitingPayment) {
            for (const auto& item : order.items()) {
                auto inventory_item = load_inventory_item(item.variant_id);
                if (!inventory_item) {
                    return Result<CancellationPlan, PlaceOrderError>::fail(inventory_item.error());
                }
                auto release_result = inventory_item.value().release(item.quantity);
                if (!release_result) {
                    return Result<CancellationPlan, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
                }
                plan.staged_inventory_items.push_back(inventory_item.value());
            }
        }

        if (status_before_cancel == OrderStatus::Paid) {
            for (const auto& item : order.items()) {
                auto inventory_item = load_inventory_item(item.variant_id);
                if (!inventory_item) {
                    return Result<CancellationPlan, PlaceOrderError>::fail(inventory_item.error());
                }
                auto restock_result = inventory_item.value().restock(item.quantity);
                if (!restock_result) {
                    return Result<CancellationPlan, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
                }
                plan.staged_inventory_items.push_back(inventory_item.value());
            }
        }

        if ((status_before_cancel == OrderStatus::AwaitingPayment ||
             status_before_cancel == OrderStatus::Paid) &&
            order.voucher_code().has_value()) {
            auto voucher = voucher_repository_.find_by_code(*order.voucher_code());
            if (!voucher.has_value()) {
                return Result<CancellationPlan, PlaceOrderError>::fail(PlaceOrderError::VoucherNotFound);
            }
            voucher->restore_use();
            plan.restored_voucher = *voucher;
        }

        return Result<CancellationPlan, PlaceOrderError>::ok(plan);
    }

    Status<OrderError> apply_order_status_transition(Order& order, OrderStatus target_status) const {
        switch (target_status) {
            case OrderStatus::Packed:
                return order.mark_packed();
            case OrderStatus::Shipped:
                return order.mark_shipped();
            case OrderStatus::Delivered:
                return order.mark_delivered();
            case OrderStatus::Completed:
                return order.mark_completed();
            default:
                return Status<OrderError>::fail(OrderError::InvalidStateTransition);
        }
    }

    void persist_inventory_items(const std::vector<InventoryItem>& items) {
        for (const auto& item : items) {
            inventory_repository_.save(item);
        }
    }

    void persist_voucher(const std::optional<Voucher>& voucher) {
        if (voucher.has_value()) {
            voucher_repository_.save(*voucher);
        }
    }

    ICartRepository& cart_repository_;
    IOrderRepository& order_repository_;
    IProductRepository& product_repository_;
    IInventoryRepository& inventory_repository_;
    IVoucherRepository& voucher_repository_;
};

}  // namespace fashion_store::application::order
