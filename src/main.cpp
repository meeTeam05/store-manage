#include <chrono>
#include <iostream>

#include "application/cart/cart_application_service.hpp"
#include "application/order/order_application_service.hpp"
#include "domain/catalog/product.hpp"
#include "domain/inventory/inventory_item.hpp"
#include "domain/order/order.hpp"
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
        "Maison Aureline Overshirt",
        Category::Outerwear,
        "Relaxed luxury overshirt for daily wear.",
        "Resort 2026");

    auto add_variant_result = product.add_variant(ProductVariantDraft{
        VariantId{"variant-001-black-m"},
        "MA-OVR-001-BLK-M",
        Size{"M"},
        Color{"Black"},
        Money::from_minor(490000)
    });
    if (!add_variant_result) {
        std::cerr << "failed to seed product variant\n";
        return 1;
    }

    product_repository.save(product);
    inventory_repository.save(InventoryItem(VariantId{"variant-001-black-m"}, 20));

    const auto now = std::chrono::system_clock::now();
    voucher_repository.save(Voucher(
        "WELCOME10",
        DiscountType::Percentage,
        10,
        Money::from_minor(300000),
        Money::from_minor(80000),
        now - std::chrono::hours(24),
        now + std::chrono::hours(24 * 30),
        100,
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

    auto cart_result = cart_service.add_item(
        cart_id,
        customer_id,
        VariantId{"variant-001-black-m"},
        2);
    if (!cart_result) {
        std::cerr << "failed to add item to cart\n";
        return 1;
    }

    auto order_result = order_service.place_order(PlaceOrderCommand{
        OrderId{"order-001"},
        cart_id,
        customer_id,
        ShippingAddress{
            "Nguyen Van A",
            "0900000000",
            "12 Nguyen Hue",
            "",
            "Ben Nghe",
            "District 1",
            "Ho Chi Minh City",
            "Vietnam"
        },
        PaymentMethod::EWallet,
        std::string("WELCOME10")
    });

    if (!order_result) {
        std::cerr << "failed to place order\n";
        return 1;
    }

    auto paid_order_result = order_service.mark_order_paid(OrderId{"order-001"});
    if (!paid_order_result) {
        std::cerr << "failed to mark order paid\n";
        return 1;
    }

    const auto& receipt = order_result.value();
    const auto& paid_order = paid_order_result.value();
    std::cout << "order_id=" << receipt.order.id().value << '\n';
    std::cout << "items=" << receipt.order.items().size() << '\n';
    std::cout << "subtotal=" << receipt.subtotal.minor_units() << '\n';
    std::cout << "discount=" << receipt.discount.minor_units() << '\n';
    std::cout << "total=" << receipt.total.minor_units() << '\n';
    std::cout << "status=" << static_cast<int>(paid_order.status()) << '\n';

    return 0;
}
