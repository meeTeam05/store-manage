#pragma once

#include <string>
#include <vector>

#include "domain/shared/types.hpp"

namespace fashion_store::domain::returns {

using fashion_store::domain::shared::OrderId;
using fashion_store::domain::shared::OrderItemId;
using fashion_store::domain::shared::Result;
using fashion_store::domain::shared::ReturnId;

enum class ReturnStatus {
    Requested,
    Approved,
    Rejected,
    Restocked,
    Refunded,
    Closed
};

enum class ReturnError {
    InvalidQuantity
};

class ReturnRequest {
public:
    static Result<ReturnRequest, ReturnError> create(ReturnId return_id,
                                                     OrderId order_id,
                                                     OrderItemId order_item_id,
                                                     int quantity,
                                                     std::string reason) {
        if (quantity <= 0) {
            return Result<ReturnRequest, ReturnError>::fail(ReturnError::InvalidQuantity);
        }
        return Result<ReturnRequest, ReturnError>::ok(
            ReturnRequest(std::move(return_id),
                          std::move(order_id),
                          std::move(order_item_id),
                          quantity,
                          std::move(reason)));
    }

    const ReturnId& id() const noexcept { return return_id_; }
    const OrderId& order_id() const noexcept { return order_id_; }
    const OrderItemId& order_item_id() const noexcept { return order_item_id_; }
    int quantity() const noexcept { return quantity_; }
    const std::string& reason() const noexcept { return reason_; }
    ReturnStatus status() const noexcept { return status_; }

private:
    ReturnRequest(ReturnId return_id, OrderId order_id, OrderItemId order_item_id, int quantity, std::string reason)
        : return_id_(std::move(return_id)),
          order_id_(std::move(order_id)),
          order_item_id_(std::move(order_item_id)),
          quantity_(quantity),
          reason_(std::move(reason)) {}

    ReturnId return_id_;
    OrderId order_id_;
    OrderItemId order_item_id_;
    int quantity_;
    std::string reason_;
    ReturnStatus status_{ReturnStatus::Requested};
};

class IReturnRepository {
public:
    virtual ~IReturnRepository() = default;

    virtual std::vector<ReturnRequest> find_by_order_id(const OrderId& order_id) const = 0;
    virtual void save(const ReturnRequest& request) = 0;
};

}  // namespace fashion_store::domain::returns
