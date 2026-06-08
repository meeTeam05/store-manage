#pragma once

#include <string>

#include "domain/order/order.hpp"
#include "domain/returns/return_request.hpp"

namespace fashion_store::domain::returns {

using fashion_store::domain::order::Order;
using fashion_store::domain::order::OrderStatus;
using fashion_store::domain::shared::OrderItemId;
using fashion_store::domain::shared::Result;
using fashion_store::domain::shared::ReturnId;

enum class ReturnEligibilityError {
    OrderNotReturnable,
    OrderItemNotFound,
    QuantityExceedsPurchased,
    InvalidReason,
    InvalidRequest
};

class ReturnPolicy {
public:
    static Result<ReturnRequest, ReturnEligibilityError> create_request_for_order_item(
        ReturnId return_id,
        const Order& order,
        const OrderItemId& order_item_id,
        int quantity,
        std::string reason) {
        if (order.status() != OrderStatus::Delivered &&
            order.status() != OrderStatus::Completed) {
            return Result<ReturnRequest, ReturnEligibilityError>::fail(ReturnEligibilityError::OrderNotReturnable);
        }
        if (reason.empty()) {
            return Result<ReturnRequest, ReturnEligibilityError>::fail(ReturnEligibilityError::InvalidReason);
        }

        const fashion_store::domain::order::OrderItem* matched_item = nullptr;
        for (const auto& item : order.items()) {
            if (item.id == order_item_id) {
                matched_item = &item;
                break;
            }
        }
        if (matched_item == nullptr) {
            return Result<ReturnRequest, ReturnEligibilityError>::fail(ReturnEligibilityError::OrderItemNotFound);
        }
        if (quantity > matched_item->quantity) {
            return Result<ReturnRequest, ReturnEligibilityError>::fail(ReturnEligibilityError::QuantityExceedsPurchased);
        }

        auto request = ReturnRequest::create(
            std::move(return_id),
            order.id(),
            order_item_id,
            quantity,
            std::move(reason));
        if (!request) {
            return Result<ReturnRequest, ReturnEligibilityError>::fail(ReturnEligibilityError::InvalidRequest);
        }
        return Result<ReturnRequest, ReturnEligibilityError>::ok(request.value());
    }
};

}  // namespace fashion_store::domain::returns
