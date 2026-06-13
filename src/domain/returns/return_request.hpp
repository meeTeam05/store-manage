#pragma once

#include <string>

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

    virtual void save(const ReturnRequest& request) = 0;
};

}  // namespace fashion_store::domain::returns
