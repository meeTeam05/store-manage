#include <cassert>

#include "application/payment/payment_application_service.hpp"
#include "domain/order/order.hpp"

int main() {
    using namespace fashion_store::application::payment;
    using namespace fashion_store::domain::order;
    using namespace fashion_store::domain::payment;
    using namespace fashion_store::domain::shared;

    Order order(
        OrderId{"order-payment-001"},
        CustomerId{"customer-payment-001"},
        {
            OrderItem{
                OrderItemId{"item-payment-001"},
                ProductId{"product-payment-001"},
                VariantId{"variant-payment-001"},
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

    PaymentApplicationService payment_service;
    auto receipt = payment_service.authorize_order_payment(order);
    assert(receipt);
    assert(receipt.value().order_id == OrderId{"order-payment-001"});
    assert(receipt.value().method == PaymentMethod::EWallet);
    assert(receipt.value().amount == Money::from_minor(4900000));
    assert(receipt.value().reference == "EWALLET-order-payment-001");

    CashPayment invalid_payment(OrderId{"order-payment-002"}, Money::from_minor(0));
    auto invalid_receipt = invalid_payment.authorize();
    assert(!invalid_receipt);
    assert(invalid_receipt.error() == PaymentError::InvalidAmount);

    return 0;
}
