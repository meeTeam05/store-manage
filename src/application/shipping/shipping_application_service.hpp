#pragma once

#include "domain/order/order.hpp"
#include "domain/shipping/shipment.hpp"

namespace fashion_store::application::shipping {

using fashion_store::domain::order::Order;
using fashion_store::domain::shared::Money;
using fashion_store::domain::shared::Result;
using fashion_store::domain::shipping::Shipment;
using fashion_store::domain::shipping::ShippingError;
using fashion_store::domain::shipping::ShippingMethod;
using fashion_store::domain::shipping::ShippingQuote;

class ShippingApplicationService {
public:
    ShippingQuote quote(ShippingMethod method) const {
        switch (method) {
            case ShippingMethod::Express:
                return ShippingQuote{method, Money::from_minor(50000), 2};
            case ShippingMethod::Standard:
            default:
                return ShippingQuote{method, Money::from_minor(25000), 5};
        }
    }

    Result<Shipment, ShippingError> create_shipment(const Order& order, ShippingMethod method) const {
        const auto shipment_quote = quote(method);
        return Shipment::create(
            order.id(),
            order.shipping_address_snapshot(),
            method,
            shipment_quote.fee,
            "SHIP-" + order.id().value);
    }
};

}  // namespace fashion_store::application::shipping
