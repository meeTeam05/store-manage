#include <cassert>
#include <chrono>
#include <optional>
#include <vector>

#include "application/cart/cart_application_service.hpp"
#include "application/order/order_application_service.hpp"
#include "domain/catalog/product.hpp"
#include "domain/inventory/inventory_item.hpp"
#include "domain/pricing/voucher.hpp"
#include "infrastructure/persistence/in_memory/in_memory_repositories.hpp"

namespace {

using namespace fashion_store::application::cart;
using namespace fashion_store::application::order;
using namespace fashion_store::domain::catalog;
using namespace fashion_store::domain::inventory;
using namespace fashion_store::domain::order;
using namespace fashion_store::domain::pricing;
using namespace fashion_store::domain::shared;
using namespace fashion_store::infrastructure::persistence::in_memory;

void seed_product(InMemoryProductRepository& product_repository,
                  const ProductId& product_id,
                  const std::vector<ProductVariantDraft>& variants) {
    Product product(product_id, "Structured Coat", Category::Outerwear, "Luxury outerwear");
    for (const auto& draft : variants) {
        auto result = product.add_variant(draft);
        assert(result);
    }
    product_repository.save(product);
}

void verify_place_order_failure_does_not_persist_partial_state() {
    InMemoryProductRepository product_repository;
    InMemoryInventoryRepository inventory_repository;
    InMemoryCartRepository cart_repository;
    InMemoryOrderRepository order_repository;
    InMemoryVoucherRepository voucher_repository;

    const auto product_id = ProductId{"product-001"};
    const auto variant_id = VariantId{"variant-001"};
    const auto cart_id = CartId{"cart-001"};
    const auto customer_id = CustomerId{"customer-001"};

    seed_product(product_repository, product_id, {
        ProductVariantDraft{variant_id, "COAT-BLK-M", Size{"M"}, Color{"Black"}, Money::from_minor(4900000)}
    });
    inventory_repository.save(InventoryItem(variant_id, 5));

    CartApplicationService cart_service(cart_repository, product_repository);
    auto add_result = cart_service.add_item(cart_id, customer_id, variant_id, 2);
    assert(add_result);

    OrderApplicationService order_service(
        cart_repository,
        order_repository,
        product_repository,
        inventory_repository,
        voucher_repository);

    auto place_result = order_service.place_order(PlaceOrderCommand{
        OrderId{"order-001"},
        cart_id,
        customer_id,
        ShippingAddress{"Client", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"},
        PaymentMethod::EWallet,
        std::string("BADCODE")
    });
    assert(!place_result);
    assert(place_result.error() == PlaceOrderError::VoucherNotFound);

    auto inventory_after = inventory_repository.find_by_variant_id(variant_id);
    assert(inventory_after.has_value());
    assert(inventory_after->on_hand() == 5);
    assert(inventory_after->reserved() == 0);

    auto cart_after = cart_repository.find_by_id(cart_id);
    assert(cart_after.has_value());
    assert(cart_after->items().size() == 1);
    assert(cart_after->items().front().quantity == 2);

    assert(!order_repository.find_by_id(OrderId{"order-001"}).has_value());
}

void verify_mark_paid_failure_does_not_commit_partial_inventory() {
    InMemoryProductRepository product_repository;
    InMemoryInventoryRepository inventory_repository;
    InMemoryCartRepository cart_repository;
    InMemoryOrderRepository order_repository;
    InMemoryVoucherRepository voucher_repository;

    const auto order_id = OrderId{"order-002"};
    const auto customer_id = CustomerId{"customer-002"};
    const auto variant_a = VariantId{"variant-a"};
    const auto variant_b = VariantId{"variant-b"};

    Order order(
        order_id,
        customer_id,
        {
            OrderItem{OrderItemId{"item-a"}, ProductId{"product-a"}, variant_a, "Item A", "SKU-A", Size{"M"}, Color{"Black"}, Money::from_minor(1000), 1, Money::from_minor(0)},
            OrderItem{OrderItemId{"item-b"}, ProductId{"product-b"}, variant_b, "Item B", "SKU-B", Size{"L"}, Color{"White"}, Money::from_minor(2000), 1, Money::from_minor(0)}
        },
        ShippingAddress{"Client", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"},
        PaymentMethod::EWallet);
    auto submit_result = order.submit_for_payment();
    assert(submit_result);
    order_repository.save(order);

    inventory_repository.save(InventoryItem(variant_a, 5, 1));

    OrderApplicationService order_service(
        cart_repository,
        order_repository,
        product_repository,
        inventory_repository,
        voucher_repository);

    auto paid_result = order_service.mark_order_paid(order_id);
    assert(!paid_result);
    assert(paid_result.error() == PlaceOrderError::InventoryNotFound);

    auto inventory_after = inventory_repository.find_by_variant_id(variant_a);
    assert(inventory_after.has_value());
    assert(inventory_after->on_hand() == 5);
    assert(inventory_after->reserved() == 1);

    auto order_after = order_repository.find_by_id(order_id);
    assert(order_after.has_value());
    assert(order_after->status() == OrderStatus::AwaitingPayment);
    assert(order_after->payment_status() == PaymentStatus::Pending);
}

void verify_cancel_failure_does_not_release_partial_inventory() {
    InMemoryProductRepository product_repository;
    InMemoryInventoryRepository inventory_repository;
    InMemoryCartRepository cart_repository;
    InMemoryOrderRepository order_repository;
    InMemoryVoucherRepository voucher_repository;

    const auto order_id = OrderId{"order-003"};
    const auto customer_id = CustomerId{"customer-003"};
    const auto variant_a = VariantId{"variant-c"};
    const auto variant_b = VariantId{"variant-d"};

    Order order(
        order_id,
        customer_id,
        {
            OrderItem{OrderItemId{"item-c"}, ProductId{"product-c"}, variant_a, "Item C", "SKU-C", Size{"M"}, Color{"Black"}, Money::from_minor(1000), 1, Money::from_minor(0)},
            OrderItem{OrderItemId{"item-d"}, ProductId{"product-d"}, variant_b, "Item D", "SKU-D", Size{"L"}, Color{"White"}, Money::from_minor(2000), 1, Money::from_minor(0)}
        },
        ShippingAddress{"Client", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"},
        PaymentMethod::EWallet);
    auto submit_result = order.submit_for_payment();
    assert(submit_result);
    order_repository.save(order);

    inventory_repository.save(InventoryItem(variant_a, 5, 1));

    OrderApplicationService order_service(
        cart_repository,
        order_repository,
        product_repository,
        inventory_repository,
        voucher_repository);

    auto cancel_result = order_service.cancel_order(order_id);
    assert(!cancel_result);
    assert(cancel_result.error() == PlaceOrderError::InventoryNotFound);

    auto inventory_after = inventory_repository.find_by_variant_id(variant_a);
    assert(inventory_after.has_value());
    assert(inventory_after->on_hand() == 5);
    assert(inventory_after->reserved() == 1);

    auto order_after = order_repository.find_by_id(order_id);
    assert(order_after.has_value());
    assert(order_after->status() == OrderStatus::AwaitingPayment);
    assert(order_after->payment_status() == PaymentStatus::Pending);
}

void verify_voucher_usage_is_consumed_and_restored() {
    InMemoryProductRepository product_repository;
    InMemoryInventoryRepository inventory_repository;
    InMemoryCartRepository cart_repository;
    InMemoryOrderRepository order_repository;
    InMemoryVoucherRepository voucher_repository;

    const auto product_id = ProductId{"product-010"};
    const auto variant_id = VariantId{"variant-010"};
    const auto cart_id = CartId{"cart-010"};
    const auto customer_id = CustomerId{"customer-010"};
    const auto now = std::chrono::system_clock::now();

    seed_product(product_repository, product_id, {
        ProductVariantDraft{variant_id, "COAT-BLK-L", Size{"L"}, Color{"Black"}, Money::from_minor(4900000)}
    });
    inventory_repository.save(InventoryItem(variant_id, 5));
    voucher_repository.save(Voucher(
        "WELCOME10",
        DiscountType::Percentage,
        10,
        Money::from_minor(1000000),
        Money::from_minor(800000),
        now - std::chrono::hours(24),
        now + std::chrono::hours(24),
        1,
        true));

    CartApplicationService cart_service(cart_repository, product_repository);
    auto add_result = cart_service.add_item(cart_id, customer_id, variant_id, 1);
    assert(add_result);

    OrderApplicationService order_service(
        cart_repository,
        order_repository,
        product_repository,
        inventory_repository,
        voucher_repository);

    auto place_result = order_service.place_order(PlaceOrderCommand{
        OrderId{"order-010"},
        cart_id,
        customer_id,
        ShippingAddress{"Client", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"},
        PaymentMethod::EWallet,
        std::string("WELCOME10")
    });
    assert(place_result);

    auto voucher_after_place = voucher_repository.find_by_code("WELCOME10");
    assert(voucher_after_place.has_value());
    assert(voucher_after_place->remaining_uses().has_value());
    assert(*voucher_after_place->remaining_uses() == 0);

    auto cancel_result = order_service.cancel_order(OrderId{"order-010"});
    assert(cancel_result);
    assert(cancel_result.value().status() == OrderStatus::Cancelled);
    assert(cancel_result.value().payment_status() == PaymentStatus::Failed);

    auto voucher_after_cancel = voucher_repository.find_by_code("WELCOME10");
    assert(voucher_after_cancel.has_value());
    assert(voucher_after_cancel->remaining_uses().has_value());
    assert(*voucher_after_cancel->remaining_uses() == 1);
}

void verify_cancel_after_shipping_is_rejected_without_side_effects() {
    InMemoryProductRepository product_repository;
    InMemoryInventoryRepository inventory_repository;
    InMemoryCartRepository cart_repository;
    InMemoryOrderRepository order_repository;
    InMemoryVoucherRepository voucher_repository;

    const auto order_id = OrderId{"order-011"};
    const auto variant_id = VariantId{"variant-011"};

    Order order(
        order_id,
        CustomerId{"customer-011"},
        {
            OrderItem{OrderItemId{"item-011"}, ProductId{"product-011"}, variant_id, "Item 011", "SKU-011", Size{"M"}, Color{"Black"}, Money::from_minor(1000), 1, Money::from_minor(0)}
        },
        ShippingAddress{"Client", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"},
        PaymentMethod::EWallet);
    assert(order.submit_for_payment());
    assert(order.mark_paid());
    assert(order.mark_packed());
    assert(order.mark_shipped());
    order_repository.save(order);
    inventory_repository.save(InventoryItem(variant_id, 4, 0));

    OrderApplicationService order_service(
        cart_repository,
        order_repository,
        product_repository,
        inventory_repository,
        voucher_repository);

    auto cancel_result = order_service.cancel_order(order_id);
    assert(!cancel_result);
    assert(cancel_result.error() == PlaceOrderError::OrderRuleViolation);

    auto stored_order = order_repository.find_by_id(order_id);
    assert(stored_order.has_value());
    assert(stored_order->status() == OrderStatus::Shipped);

    auto inventory_after = inventory_repository.find_by_variant_id(variant_id);
    assert(inventory_after.has_value());
    assert(inventory_after->on_hand() == 4);
    assert(inventory_after->reserved() == 0);
}

void verify_cancel_paid_order_marks_payment_refunded() {
    InMemoryProductRepository product_repository;
    InMemoryInventoryRepository inventory_repository;
    InMemoryCartRepository cart_repository;
    InMemoryOrderRepository order_repository;
    InMemoryVoucherRepository voucher_repository;

    const auto order_id = OrderId{"order-012"};
    const auto variant_id = VariantId{"variant-012"};

    Order order(
        order_id,
        CustomerId{"customer-012"},
        {
            OrderItem{OrderItemId{"item-012"}, ProductId{"product-012"}, variant_id, "Item 012", "SKU-012", Size{"L"}, Color{"Ivory"}, Money::from_minor(2000), 2, Money::from_minor(0)}
        },
        ShippingAddress{"Client", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"},
        PaymentMethod::EWallet);
    assert(order.submit_for_payment());
    assert(order.mark_paid());
    order_repository.save(order);
    inventory_repository.save(InventoryItem(variant_id, 7, 0));

    OrderApplicationService order_service(
        cart_repository,
        order_repository,
        product_repository,
        inventory_repository,
        voucher_repository);

    auto cancel_result = order_service.cancel_order(order_id);
    assert(cancel_result);
    assert(cancel_result.value().status() == OrderStatus::Cancelled);
    assert(cancel_result.value().payment_status() == PaymentStatus::Refunded);

    auto inventory_after = inventory_repository.find_by_variant_id(variant_id);
    assert(inventory_after.has_value());
    assert(inventory_after->on_hand() == 9);
    assert(inventory_after->reserved() == 0);
}

}  // namespace

int main() {
    verify_place_order_failure_does_not_persist_partial_state();
    verify_mark_paid_failure_does_not_commit_partial_inventory();
    verify_cancel_failure_does_not_release_partial_inventory();
    verify_voucher_usage_is_consumed_and_restored();
    verify_cancel_after_shipping_is_rejected_without_side_effects();
    verify_cancel_paid_order_marks_payment_refunded();
    return 0;
}
