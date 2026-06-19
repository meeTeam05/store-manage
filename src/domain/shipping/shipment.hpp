#pragma once

#include <string>
#include <utility>

#include "domain/shared/types.hpp"

namespace fashion_store::domain::shipping {

using fashion_store::domain::shared::Money;
using fashion_store::domain::shared::OrderId;
using fashion_store::domain::shared::Result;
using fashion_store::domain::shared::ShippingAddress;
using fashion_store::domain::shared::Status;
using fashion_store::domain::shared::ok_status;

enum class ShippingMethod {
    Standard,
    Express
};

enum class ShippingStatus {
    Preparing,
    InTransit,
    Delivered,
    Failed
};

enum class ShippingError {
    InvalidAddress,
    InvalidShipmentData,
    InvalidStateTransition
};

struct ShippingQuote {
    ShippingMethod method;
    Money fee;
    int estimated_days;
};

class Shipment {
public:
    static Result<Shipment, ShippingError> create(OrderId order_id,
                                                  ShippingAddress address,
                                                  ShippingMethod method,
                                                  Money fee,
                                                  std::string tracking_code) {
        if (address.recipient_name.empty() || address.phone.empty() ||
            address.line1.empty() || address.city.empty() || address.country.empty()) {
            return Result<Shipment, ShippingError>::fail(ShippingError::InvalidAddress);
        }
        if (tracking_code.empty() || fee < Money::from_minor(0)) {
            return Result<Shipment, ShippingError>::fail(ShippingError::InvalidShipmentData);
        }
        return Result<Shipment, ShippingError>::ok(Shipment(
            std::move(order_id),
            std::move(address),
            method,
            fee,
            std::move(tracking_code)));
    }

    const OrderId& order_id() const noexcept { return order_id_; }
    const ShippingAddress& address() const noexcept { return address_; }
    ShippingMethod method() const noexcept { return method_; }
    ShippingStatus status() const noexcept { return status_; }
    Money fee() const noexcept { return fee_; }
    const std::string& tracking_code() const noexcept { return tracking_code_; }

    Status<ShippingError> mark_in_transit() {
        if (status_ != ShippingStatus::Preparing) {
            return Status<ShippingError>::fail(ShippingError::InvalidStateTransition);
        }
        status_ = ShippingStatus::InTransit;
        return ok_status<ShippingError>();
    }

    Status<ShippingError> mark_delivered() {
        if (status_ != ShippingStatus::InTransit) {
            return Status<ShippingError>::fail(ShippingError::InvalidStateTransition);
        }
        status_ = ShippingStatus::Delivered;
        return ok_status<ShippingError>();
    }

    Status<ShippingError> mark_failed() {
        if (status_ == ShippingStatus::Delivered) {
            return Status<ShippingError>::fail(ShippingError::InvalidStateTransition);
        }
        status_ = ShippingStatus::Failed;
        return ok_status<ShippingError>();
    }

private:
    Shipment(OrderId order_id,
             ShippingAddress address,
             ShippingMethod method,
             Money fee,
             std::string tracking_code)
        : order_id_(std::move(order_id)),
          address_(std::move(address)),
          method_(method),
          fee_(fee),
          tracking_code_(std::move(tracking_code)) {}

    OrderId order_id_;
    ShippingAddress address_;
    ShippingMethod method_;
    ShippingStatus status_{ShippingStatus::Preparing};
    Money fee_;
    std::string tracking_code_;
};

}  // namespace fashion_store::domain::shipping
