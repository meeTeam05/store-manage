#include <cassert>

#include "application/shipping/shipping_application_service.hpp"
#include "domain/order/order.hpp"

int main() {
    using namespace fashion_store::application::shipping;
    using namespace fashion_store::domain::order;
    using namespace fashion_store::domain::shared;
    using namespace fashion_store::domain::shipping;

    Order order(
        OrderId{"order-shipping-001"},
        CustomerId{"customer-shipping-001"},
        {
            OrderItem{
                OrderItemId{"item-shipping-001"},
                ProductId{"product-shipping-001"},
                VariantId{"variant-shipping-001"},
                "Structured Coat",
                "COAT-BLK-M",
                Size{"M"},
                Color{"Black"},
                Money::from_minor(4900000),
                1,
                Money::from_minor(0)}
        },
        ShippingAddress{"Client", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"},
        PaymentMethod::EWallet);

    ShippingApplicationService shipping_service;
    auto express_quote = shipping_service.quote(ShippingMethod::Express);
    assert(express_quote.fee == Money::from_minor(50000));
    assert(express_quote.estimated_days == 2);

    auto shipment = shipping_service.create_shipment(order, ShippingMethod::Express);
    assert(shipment);
    assert(shipment.value().tracking_code() == "SHIP-order-shipping-001");
    assert(shipment.value().status() == ShippingStatus::Preparing);
    assert(shipment.value().mark_in_transit());
    assert(shipment.value().status() == ShippingStatus::InTransit);
    assert(shipment.value().mark_delivered());
    assert(shipment.value().status() == ShippingStatus::Delivered);
    assert(!shipment.value().mark_failed());

    auto invalid = Shipment::create(
        OrderId{"order-shipping-002"},
        ShippingAddress{},
        ShippingMethod::Standard,
        Money::from_minor(25000),
        "SHIP-order-shipping-002");
    assert(!invalid);
    assert(invalid.error() == ShippingError::InvalidAddress);

    auto invalid_tracking = Shipment::create(
        OrderId{"order-shipping-003"},
        ShippingAddress{"Client", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"},
        ShippingMethod::Standard,
        Money::from_minor(25000),
        "");
    assert(!invalid_tracking);
    assert(invalid_tracking.error() == ShippingError::InvalidShipmentData);

    auto invalid_fee = Shipment::create(
        OrderId{"order-shipping-004"},
        ShippingAddress{"Client", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"},
        ShippingMethod::Express,
        Money::from_minor(-1),
        "SHIP-order-shipping-004");
    assert(!invalid_fee);
    assert(invalid_fee.error() == ShippingError::InvalidShipmentData);

    return 0;
}
