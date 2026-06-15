#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "domain/shared/types.hpp"

namespace fashion_store::domain::returns {

using fashion_store::domain::shared::OrderId;
using fashion_store::domain::shared::OrderItemId;
using fashion_store::domain::shared::Result;
using fashion_store::domain::shared::ReturnId;
using fashion_store::domain::shared::Status;
using fashion_store::domain::shared::ok_status;

enum class ReturnStatus {
    Requested,
    Approved,
    Rejected,
    Restocked,
    Refunded,
    Closed
};

enum class ReturnError {
    InvalidQuantity,
    InvalidStateTransition
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

    Status<ReturnError> approve() {
        if (status_ != ReturnStatus::Requested) {
            return Status<ReturnError>::fail(ReturnError::InvalidStateTransition);
        }
        status_ = ReturnStatus::Approved;
        return ok_status<ReturnError>();
    }

    Status<ReturnError> reject() {
        if (status_ != ReturnStatus::Requested) {
            return Status<ReturnError>::fail(ReturnError::InvalidStateTransition);
        }
        status_ = ReturnStatus::Rejected;
        return ok_status<ReturnError>();
    }

    Status<ReturnError> mark_restocked() {
        if (status_ != ReturnStatus::Approved) {
            return Status<ReturnError>::fail(ReturnError::InvalidStateTransition);
        }
        status_ = ReturnStatus::Restocked;
        return ok_status<ReturnError>();
    }

    Status<ReturnError> mark_refunded() {
        if (status_ != ReturnStatus::Restocked) {
            return Status<ReturnError>::fail(ReturnError::InvalidStateTransition);
        }
        status_ = ReturnStatus::Refunded;
        return ok_status<ReturnError>();
    }

    Status<ReturnError> close() {
        if (status_ != ReturnStatus::Rejected &&
            status_ != ReturnStatus::Refunded) {
            return Status<ReturnError>::fail(ReturnError::InvalidStateTransition);
        }
        status_ = ReturnStatus::Closed;
        return ok_status<ReturnError>();
    }

    static ReturnRequest rehydrate(ReturnId return_id,
                                   OrderId order_id,
                                   OrderItemId order_item_id,
                                   int quantity,
                                   std::string reason,
                                   ReturnStatus status) {
        auto request = ReturnRequest(
            std::move(return_id),
            std::move(order_id),
            std::move(order_item_id),
            quantity,
            std::move(reason));
        request.status_ = status;
        return request;
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

    virtual std::optional<ReturnRequest> find_by_id(const ReturnId& return_id) const = 0;
    virtual std::vector<ReturnRequest> find_by_order_id(const OrderId& order_id) const = 0;
    virtual void save(const ReturnRequest& request) = 0;
};

}  // namespace fashion_store::domain::returns
