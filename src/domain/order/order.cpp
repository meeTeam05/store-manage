#include "domain/order/order.hpp"

namespace fashion_store::domain::order {

Order::Order(OrderId id,
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

Money Order::subtotal() const noexcept {
    auto total = Money::from_minor(0);
    for (const auto& item : items_) {
        total = total + item.unit_price.multiply(item.quantity);
    }
    return total;
}

Money Order::total() const noexcept {
    return subtotal() - discount_total_;
}

Status<OrderError> Order::apply_discount(Money discount, std::optional<std::string> voucher_code) {
    if (discount < Money::from_minor(0) || discount > subtotal()) {
        return Status<OrderError>::fail(OrderError::InvalidDiscount);
    }
    discount_total_ = discount;
    voucher_code_ = std::move(voucher_code);
    return ok_status<OrderError>();
}

Status<OrderError> Order::submit_for_payment() {
    if (status_ != OrderStatus::Draft) {
        return Status<OrderError>::fail(OrderError::InvalidStateTransition);
    }
    status_ = OrderStatus::AwaitingPayment;
    payment_status_ = PaymentStatus::Pending;
    return ok_status<OrderError>();
}

Status<OrderError> Order::mark_paid() {
    if (status_ != OrderStatus::AwaitingPayment) {
        return Status<OrderError>::fail(OrderError::InvalidStateTransition);
    }
    status_ = OrderStatus::Paid;
    payment_status_ = PaymentStatus::Paid;
    return ok_status<OrderError>();
}

Status<OrderError> Order::mark_packed() {
    if (status_ != OrderStatus::Paid) {
        return Status<OrderError>::fail(OrderError::InvalidStateTransition);
    }
    status_ = OrderStatus::Packed;
    return ok_status<OrderError>();
}

Status<OrderError> Order::mark_shipped() {
    if (status_ != OrderStatus::Packed) {
        return Status<OrderError>::fail(OrderError::InvalidStateTransition);
    }
    status_ = OrderStatus::Shipped;
    return ok_status<OrderError>();
}

Status<OrderError> Order::mark_delivered() {
    if (status_ != OrderStatus::Shipped) {
        return Status<OrderError>::fail(OrderError::InvalidStateTransition);
    }
    status_ = OrderStatus::Delivered;
    return ok_status<OrderError>();
}

Status<OrderError> Order::mark_completed() {
    if (status_ != OrderStatus::Delivered) {
        return Status<OrderError>::fail(OrderError::InvalidStateTransition);
    }
    status_ = OrderStatus::Completed;
    return ok_status<OrderError>();
}

Status<OrderError> Order::cancel() {
    if (status_ != OrderStatus::Draft &&
        status_ != OrderStatus::AwaitingPayment &&
        status_ != OrderStatus::Paid) {
        return Status<OrderError>::fail(OrderError::InvalidStateTransition);
    }
    status_ = OrderStatus::Cancelled;
    return ok_status<OrderError>();
}

}  // namespace fashion_store::domain::order
