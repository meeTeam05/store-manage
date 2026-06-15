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
          PaymentMethod payment_method);

    const OrderId& id() const noexcept { return id_; }
    const CustomerId& customer_id() const noexcept { return customer_id_; }
    const std::vector<OrderItem>& items() const noexcept { return items_; }
    OrderStatus status() const noexcept { return status_; }
    PaymentStatus payment_status() const noexcept { return payment_status_; }

    Money subtotal() const noexcept;

    Money total() const noexcept;

    Status<OrderError> apply_discount(Money discount, std::optional<std::string> voucher_code = std::nullopt);

    Status<OrderError> submit_for_payment();

    Status<OrderError> mark_paid();

    Status<OrderError> mark_packed();

    Status<OrderError> mark_shipped();

    Status<OrderError> mark_delivered();

    Status<OrderError> mark_completed();

    Status<OrderError> cancel();

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
