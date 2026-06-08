#include <cassert>
#include <chrono>

#include "application/cart/cart_application_service.hpp"
#include "application/order/order_application_service.hpp"
#include "domain/catalog/product.hpp"
#include "domain/inventory/inventory_item.hpp"
#include "domain/pricing/voucher.hpp"
#include "infrastructure/persistence/in_memory/in_memory_repositories.hpp"

int main() {
    using namespace fashion_store::application::cart;
    using namespace fashion_store::application::order;
    using namespace fashion_store::domain::catalog;
    using namespace fashion_store::domain::inventory;
    using namespace fashion_store::domain::order;
    using namespace fashion_store::domain::pricing;
    using namespace fashion_store::domain::shared;
    using namespace fashion_store::infrastructure::persistence::in_memory;

    InMemoryProductRepository product_repository;
    InMemoryInventoryRepository inventory_repository;
    InMemoryCartRepository cart_repository;
    InMemoryOrderRepository order_repository;
    InMemoryVoucherRepository voucher_repository;

    Product product(
        ProductId{"product-001"},
        "Structured Coat",
        Category::Outerwear,
        "Luxury outerwear");
    auto add_variant = product.add_variant(ProductVariantDraft{
        VariantId{"variant-001"},
        "COAT-BLK-M",
        Size{"M"},
        Color{"Black"},
        Money::from_minor(4900000)
    });
    assert(add_variant);
    product_repository.save(product);
    inventory_repository.save(InventoryItem(VariantId{"variant-001"}, 5));

    const auto now = std::chrono::system_clock::now();
    voucher_repository.save(Voucher(
        "WELCOME10",
        DiscountType::Percentage,
        10,
        Money::from_minor(1000000),
        Money::from_minor(800000),
        now - std::chrono::hours(24),
        now + std::chrono::hours(24),
        10,
        true));

    CartApplicationService cart_service(cart_repository, product_repository);
    OrderApplicationService order_service(
        cart_repository,
        order_repository,
        product_repository,
        inventory_repository,
        voucher_repository);

    const auto customer_id = CustomerId{"customer-001"};
    const auto cart_id = CartId{"cart-001"};

    auto add_to_cart = cart_service.add_item(cart_id, customer_id, VariantId{"variant-001"}, 2);
    assert(add_to_cart);
    assert(add_to_cart.value().subtotal() == Money::from_minor(9800000));

    auto place_order = order_service.place_order(PlaceOrderCommand{
        OrderId{"order-001"},
        cart_id,
        customer_id,
        ShippingAddress{"Client", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"},
        PaymentMethod::EWallet,
        std::string("WELCOME10")
    });
    assert(place_order);
    assert(place_order.value().discount == Money::from_minor(800000));
    assert(place_order.value().total == Money::from_minor(9000000));

    auto paid = order_service.mark_order_paid(OrderId{"order-001"});
    assert(paid);
    assert(paid.value().status() == OrderStatus::Paid);

    auto packed = order_service.advance_order_status(OrderId{"order-001"}, OrderStatus::Packed);
    assert(packed);
    auto shipped = order_service.advance_order_status(OrderId{"order-001"}, OrderStatus::Shipped);
    assert(shipped);
    auto delivered = order_service.advance_order_status(OrderId{"order-001"}, OrderStatus::Delivered);
    assert(delivered);
    auto completed = order_service.advance_order_status(OrderId{"order-001"}, OrderStatus::Completed);
    assert(completed);

    return 0;
}
