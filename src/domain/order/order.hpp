#pragma once

#include <optional>
#include <string>
#include <vector>

#include "domain/shared/types.hpp"

namespace fashion_store::domain::order {

using fashion_store::domain::shared::Color;
using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::Money;
using fashion_store::domain::shared::OrderId;
using fashion_store::domain::shared::OrderItemId;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::ShippingAddress;
using fashion_store::domain::shared::Size;
using fashion_store::domain::shared::Status;
using fashion_store::domain::shared::VariantId;
using fashion_store::domain::shared::ok_status;
using fashion_store::domain::shared::require;

enum class OrderStatus {
    Draft,
    AwaitingPayment,
    Paid,
    Packed,
    Shipped,
    Delivered,
    Completed,
    Cancelled,
    Returned
};

enum class PaymentMethod {
    Cash,
    BankTransfer,
    EWallet
};

enum class PaymentStatus {
    Unpaid,
    Pending,
    Paid,
    Failed,
    Refunded
};

enum class OrderError {
    EmptyOrder,
    InvalidDiscount,
    InvalidStateTransition
};

struct OrderItem {
    OrderItemId id;
    ProductId product_id;
    VariantId variant_id;
    std::string product_name;
    std::string sku;
    Size size;
    Color color;
    Money unit_price;
    int quantity;
    Money discount_amount;

    Money line_total() const noexcept {
        return unit_price.multiply(quantity) - discount_amount;
    }
};

class Order {
public:
    Order(OrderId id,
          CustomerId customer_id,
          std::vector<OrderItem> items,
          ShippingAddress shipping_address_snapshot,
          PaymentMethod payment_method)
        : id_(std::move(id)),
          customer_id_(std::move(customer_id)),
          items_(std::move(items)),
          shipping_address_snapshot_(std::move(shipping_address_snapshot)),
          payment_method_(payment_method) {
        require(!id_.value.empty(), "order id must not be empty");
        require(!customer_id_.value.empty(), "customer id must not be empty");
        require(!items_.empty(), "order must contain at least one item");
    }

    const OrderId& id() const noexcept { return id_; }
    const CustomerId& customer_id() const noexcept { return customer_id_; }
    const std::vector<OrderItem>& items() const noexcept { return items_; }
    OrderStatus status() const noexcept { return status_; }
    PaymentStatus payment_status() const noexcept { return payment_status_; }

    Money subtotal() const noexcept {
        auto total = Money::from_minor(0);
        for (const auto& item : items_) {
            total = total + item.unit_price.multiply(item.quantity);
        }
        return total;
    }

    Money total() const noexcept {
        return subtotal() - discount_total_;
    }

    Status<OrderError> apply_discount(Money discount, std::optional<std::string> voucher_code = std::nullopt) {
        if (discount < Money::from_minor(0) || discount > subtotal()) {
            return Status<OrderError>::fail(OrderError::InvalidDiscount);
        }
        discount_total_ = discount;
        voucher_code_ = std::move(voucher_code);
        return ok_status<OrderError>();
    }

    Status<OrderError> submit_for_payment() {
        if (status_ != OrderStatus::Draft) {
            return Status<OrderError>::fail(OrderError::InvalidStateTransition);
        }
        status_ = OrderStatus::AwaitingPayment;
        payment_status_ = PaymentStatus::Pending;
        return ok_status<OrderError>();
    }

    Status<OrderError> mark_paid() {
        if (status_ != OrderStatus::AwaitingPayment) {
            return Status<OrderError>::fail(OrderError::InvalidStateTransition);
        }
        status_ = OrderStatus::Paid;
        payment_status_ = PaymentStatus::Paid;
        return ok_status<OrderError>();
    }

    Status<OrderError> mark_packed() {
        if (status_ != OrderStatus::Paid) {
            return Status<OrderError>::fail(OrderError::InvalidStateTransition);
        }
        status_ = OrderStatus::Packed;
        return ok_status<OrderError>();
    }

    Status<OrderError> mark_shipped() {
        if (status_ != OrderStatus::Packed) {
            return Status<OrderError>::fail(OrderError::InvalidStateTransition);
        }
        status_ = OrderStatus::Shipped;
        return ok_status<OrderError>();
    }

    Status<OrderError> mark_delivered() {
        if (status_ != OrderStatus::Shipped) {
            return Status<OrderError>::fail(OrderError::InvalidStateTransition);
        }
        status_ = OrderStatus::Delivered;
        return ok_status<OrderError>();
    }

    Status<OrderError> mark_completed() {
        if (status_ != OrderStatus::Delivered) {
            return Status<OrderError>::fail(OrderError::InvalidStateTransition);
        }
        status_ = OrderStatus::Completed;
        return ok_status<OrderError>();
    }

    Status<OrderError> cancel() {
        if (status_ != OrderStatus::Draft &&
            status_ != OrderStatus::AwaitingPayment &&
            status_ != OrderStatus::Paid) {
            return Status<OrderError>::fail(OrderError::InvalidStateTransition);
        }
        status_ = OrderStatus::Cancelled;
        return ok_status<OrderError>();
    }

private:
    OrderId id_;
    CustomerId customer_id_;
    std::vector<OrderItem> items_;
    ShippingAddress shipping_address_snapshot_;
    OrderStatus status_{OrderStatus::Draft};
    PaymentMethod payment_method_{PaymentMethod::Cash};
    PaymentStatus payment_status_{PaymentStatus::Unpaid};
    std::optional<std::string> voucher_code_;
    Money discount_total_{Money::from_minor(0)};
};

}  // namespace fashion_store::domain::order
