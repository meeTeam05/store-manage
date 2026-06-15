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

using fashion_store::domain::cart::ICartRepository;
using fashion_store::domain::catalog::IProductRepository;
using fashion_store::domain::inventory::IInventoryRepository;
using fashion_store::domain::order::IOrderRepository;
using fashion_store::domain::order::Order;
using fashion_store::domain::order::OrderStatus;
using fashion_store::domain::order::OrderItem;
using fashion_store::domain::order::PaymentMethod;
using fashion_store::domain::pricing::IVoucherRepository;
using fashion_store::domain::shared::CartId;
using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::Money;
using fashion_store::domain::shared::OrderId;
using fashion_store::domain::shared::OrderItemId;
using fashion_store::domain::shared::Result;
using fashion_store::domain::shared::ShippingAddress;

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
        auto cart = cart_repository_.find_by_id(cart_id);
        if (!cart.has_value()) {
            return Result<CheckoutPreview, PlaceOrderError>::fail(PlaceOrderError::CartNotFound);
        }
        if (cart->empty()) {
            return Result<CheckoutPreview, PlaceOrderError>::fail(PlaceOrderError::EmptyCart);
        }

        for (const auto& item : cart->items()) {
            auto variant_view = product_repository_.find_variant(item.variant_id);
            if (!variant_view.has_value()) {
                return Result<CheckoutPreview, PlaceOrderError>::fail(PlaceOrderError::ProductVariantNotFound);
            }
            auto inventory_item = inventory_repository_.find_by_variant_id(item.variant_id);
            if (!inventory_item.has_value()) {
                return Result<CheckoutPreview, PlaceOrderError>::fail(PlaceOrderError::InventoryNotFound);
            }
            if (inventory_item->available() < item.quantity) {
                return Result<CheckoutPreview, PlaceOrderError>::fail(PlaceOrderError::InsufficientStock);
            }
        }

        const auto subtotal = cart->subtotal();
        auto discount = Money::from_minor(0);
        bool voucher_applied = false;
        if (voucher_code.has_value()) {
            auto voucher = voucher_repository_.find_by_code(*voucher_code);
            if (!voucher.has_value()) {
                return Result<CheckoutPreview, PlaceOrderError>::fail(PlaceOrderError::VoucherNotFound);
            }
            auto voucher_validation = voucher->validate(subtotal, std::chrono::system_clock::now());
            if (!voucher_validation) {
                return Result<CheckoutPreview, PlaceOrderError>::fail(PlaceOrderError::VoucherInvalid);
            }
            discount = voucher->calculate_discount(subtotal);
            voucher_applied = true;
        }

        return Result<CheckoutPreview, PlaceOrderError>::ok(
            CheckoutPreview{cart->items().size(), subtotal, discount, subtotal - discount, voucher_applied});
    }

    Result<PlaceOrderReceipt, PlaceOrderError> place_order(const PlaceOrderCommand& command) {
        auto cart = cart_repository_.find_by_id(command.cart_id);
        if (!cart.has_value()) {
            return Result<PlaceOrderReceipt, PlaceOrderError>::fail(PlaceOrderError::CartNotFound);
        }
        if (cart->empty()) {
            return Result<PlaceOrderReceipt, PlaceOrderError>::fail(PlaceOrderError::EmptyCart);
        }

        std::vector<OrderItem> order_items;
        std::vector<fashion_store::domain::inventory::InventoryItem> staged_inventory_items;
        for (const auto& item : cart->items()) {
            auto variant_view = product_repository_.find_variant(item.variant_id);
            if (!variant_view.has_value()) {
                return Result<PlaceOrderReceipt, PlaceOrderError>::fail(PlaceOrderError::ProductVariantNotFound);
            }

            auto inventory_item = inventory_repository_.find_by_variant_id(item.variant_id);
            if (!inventory_item.has_value()) {
                return Result<PlaceOrderReceipt, PlaceOrderError>::fail(PlaceOrderError::InventoryNotFound);
            }

            auto reserve_result = inventory_item->reserve(item.quantity);
            if (!reserve_result) {
                return Result<PlaceOrderReceipt, PlaceOrderError>::fail(PlaceOrderError::InsufficientStock);
            }
            staged_inventory_items.push_back(*inventory_item);

            order_items.push_back(OrderItem{
                OrderItemId{command.order_id.value + "-" + item.variant_id.value},
                variant_view->product_id,
                variant_view->variant.id,
                variant_view->product_name,
                variant_view->variant.sku,
                variant_view->variant.size,
                variant_view->variant.color,
                variant_view->variant.price,
                item.quantity,
                Money::from_minor(0)
            });
        }

        Order order(command.order_id,
                    command.customer_id,
                    order_items,
                    command.shipping_address,
                    command.payment_method);

        const auto subtotal = order.subtotal();
        auto discount = Money::from_minor(0);
        std::optional<fashion_store::domain::pricing::Voucher> staged_voucher;

        if (command.voucher_code.has_value()) {
            auto voucher = voucher_repository_.find_by_code(*command.voucher_code);
            if (!voucher.has_value()) {
                return Result<PlaceOrderReceipt, PlaceOrderError>::fail(PlaceOrderError::VoucherNotFound);
            }
            auto voucher_validation = voucher->validate(subtotal, std::chrono::system_clock::now());
            if (!voucher_validation) {
                return Result<PlaceOrderReceipt, PlaceOrderError>::fail(PlaceOrderError::VoucherInvalid);
            }
            discount = voucher->calculate_discount(subtotal);
            auto consume_result = voucher->consume();
            if (!consume_result) {
                return Result<PlaceOrderReceipt, PlaceOrderError>::fail(PlaceOrderError::VoucherInvalid);
            }
            staged_voucher = *voucher;
        }

        auto discount_result = order.apply_discount(discount, command.voucher_code);
        if (!discount_result) {
            return Result<PlaceOrderReceipt, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }

        auto submit_result = order.submit_for_payment();
        if (!submit_result) {
            return Result<PlaceOrderReceipt, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }

        auto cleared_cart = *cart;
        cleared_cart.clear();

        persist_inventory_items(staged_inventory_items);
        persist_voucher(staged_voucher);
        order_repository_.save(order);
        cart_repository_.save(cleared_cart);

        return Result<PlaceOrderReceipt, PlaceOrderError>::ok(
            PlaceOrderReceipt{order, subtotal, discount, order.total()});
    }

    Result<Order, PlaceOrderError> mark_order_paid(const OrderId& order_id) {
        auto order = order_repository_.find_by_id(order_id);
        if (!order.has_value()) {
            return Result<Order, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }

        std::vector<fashion_store::domain::inventory::InventoryItem> staged_inventory_items;
        for (const auto& item : order->items()) {
            auto inventory_item = inventory_repository_.find_by_variant_id(item.variant_id);
            if (!inventory_item.has_value()) {
                return Result<Order, PlaceOrderError>::fail(PlaceOrderError::InventoryNotFound);
            }

            auto commit_result = inventory_item->commit_sale(item.quantity);
            if (!commit_result) {
                return Result<Order, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
            }
            staged_inventory_items.push_back(*inventory_item);
        }

        auto paid_order = *order;
        auto paid_result = paid_order.mark_paid();
        if (!paid_result) {
            return Result<Order, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }

        persist_inventory_items(staged_inventory_items);
        order_repository_.save(paid_order);
        return Result<Order, PlaceOrderError>::ok(paid_order);
    }

    Result<Order, PlaceOrderError> advance_order_status(const OrderId& order_id, OrderStatus target_status) {
        auto order = order_repository_.find_by_id(order_id);
        if (!order.has_value()) {
            return Result<Order, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }

        auto result = [&]() {
            switch (target_status) {
                case OrderStatus::Packed:
                    return order->mark_packed();
                case OrderStatus::Shipped:
                    return order->mark_shipped();
                case OrderStatus::Delivered:
                    return order->mark_delivered();
                case OrderStatus::Completed:
                    return order->mark_completed();
                default:
                    return fashion_store::domain::shared::Status<fashion_store::domain::order::OrderError>::fail(
                        fashion_store::domain::order::OrderError::InvalidStateTransition);
            }
        }();

        if (!result) {
            return Result<Order, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }

        order_repository_.save(*order);
        return Result<Order, PlaceOrderError>::ok(*order);
    }

    Result<Order, PlaceOrderError> cancel_order(const OrderId& order_id) {
        auto order = order_repository_.find_by_id(order_id);
        if (!order.has_value()) {
            return Result<Order, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }

        const auto status_before_cancel = order->status();
        std::vector<fashion_store::domain::inventory::InventoryItem> staged_inventory_items;
        std::optional<fashion_store::domain::pricing::Voucher> restored_voucher;
        if (status_before_cancel == OrderStatus::AwaitingPayment) {
            for (const auto& item : order->items()) {
                auto inventory_item = inventory_repository_.find_by_variant_id(item.variant_id);
                if (!inventory_item.has_value()) {
                    return Result<Order, PlaceOrderError>::fail(PlaceOrderError::InventoryNotFound);
                }
                auto release_result = inventory_item->release(item.quantity);
                if (!release_result) {
                    return Result<Order, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
                }
                staged_inventory_items.push_back(*inventory_item);
            }
        }

        if (status_before_cancel == OrderStatus::Paid) {
            for (const auto& item : order->items()) {
                auto inventory_item = inventory_repository_.find_by_variant_id(item.variant_id);
                if (!inventory_item.has_value()) {
                    return Result<Order, PlaceOrderError>::fail(PlaceOrderError::InventoryNotFound);
                }
                auto restock_result = inventory_item->restock(item.quantity);
                if (!restock_result) {
                    return Result<Order, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
                }
                staged_inventory_items.push_back(*inventory_item);
            }
        }

        if ((status_before_cancel == OrderStatus::AwaitingPayment ||
             status_before_cancel == OrderStatus::Paid) &&
            order->voucher_code().has_value()) {
            auto voucher = voucher_repository_.find_by_code(*order->voucher_code());
            if (!voucher.has_value()) {
                return Result<Order, PlaceOrderError>::fail(PlaceOrderError::VoucherNotFound);
            }
            voucher->restore_use();
            restored_voucher = *voucher;
        }

        auto cancelled_order = *order;
        auto cancel_result = cancelled_order.cancel();
        if (!cancel_result) {
            return Result<Order, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }

        persist_inventory_items(staged_inventory_items);
        persist_voucher(restored_voucher);
        order_repository_.save(cancelled_order);
        return Result<Order, PlaceOrderError>::ok(cancelled_order);
    }

    Result<Order, PlaceOrderError> get_order(const OrderId& order_id) const {
        auto order = order_repository_.find_by_id(order_id);
        if (!order.has_value()) {
            return Result<Order, PlaceOrderError>::fail(PlaceOrderError::OrderRuleViolation);
        }
        return Result<Order, PlaceOrderError>::ok(*order);
    }

    std::vector<Order> get_customer_orders(const CustomerId& customer_id) const {
        return order_repository_.find_by_customer_id(customer_id);
    }

private:
    void persist_inventory_items(
        const std::vector<fashion_store::domain::inventory::InventoryItem>& items) {
        for (const auto& item : items) {
            inventory_repository_.save(item);
        }
    }

    void persist_voucher(const std::optional<fashion_store::domain::pricing::Voucher>& voucher) {
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
