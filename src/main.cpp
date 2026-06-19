#include <chrono>
#include <iostream>
#include <optional>

#include "application/cart/cart_application_service.hpp"
#include "application/customer/customer_application_service.hpp"
#include "application/identity/auth_application_service.hpp"
#include "application/order/order_application_service.hpp"
#include "application/review/review_application_service.hpp"
#include "application/returns/returns_application_service.hpp"
#include "domain/catalog/product.hpp"
#include "domain/customer/customer.hpp"
#include "domain/identity/account.hpp"
#include "domain/inventory/inventory_item.hpp"
#include "domain/order/order.hpp"
#include "domain/pricing/voucher.hpp"
#include "infrastructure/persistence/in_memory/in_memory_repositories.hpp"

int main() {
    using namespace fashion_store::application::cart;
    using namespace fashion_store::application::customer;
    using namespace fashion_store::application::identity;
    using namespace fashion_store::application::order;
    using namespace fashion_store::application::review;
    using namespace fashion_store::application::returns;
    using namespace fashion_store::domain::catalog;
    using namespace fashion_store::domain::customer;
    using namespace fashion_store::domain::identity;
    using namespace fashion_store::domain::inventory;
    using namespace fashion_store::domain::order;
    using namespace fashion_store::domain::pricing;
    using namespace fashion_store::domain::shared;
    using namespace fashion_store::infrastructure::persistence::in_memory;

    InMemoryProductRepository product_repository;
    InMemoryCustomerRepository customer_repository;
    InMemoryAccountRepository account_repository;
    InMemoryInventoryRepository inventory_repository;
    InMemoryCartRepository cart_repository;
    InMemoryOrderRepository order_repository;
    InMemoryVoucherRepository voucher_repository;
    InMemoryReviewRepository review_repository;
    InMemoryReturnRepository return_repository;

    Product product(
        ProductId{"product-001"},
        "Structured House Overshirt",
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

    const auto customer_id = CustomerId{"customer-001"};
    customer_repository.save(Customer(
        customer_id,
        AccountId{"account-001"},
        "Nguyen Van A",
        "0900000000",
        ShippingAddress{
            "Nguyen Van A",
            "0900000000",
            "12 Nguyen Hue",
            "",
            "Ben Nghe",
            "District 1",
            "Ho Chi Minh City",
            "Vietnam"
        }));
    account_repository.save(Account(
        AccountId{"account-001"},
        "client001",
        "hash:123456",
        Role::Customer,
        AccountStatus::Active));

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
    CustomerApplicationService customer_service(customer_repository);
    AuthApplicationService auth_service(account_repository);
    OrderApplicationService order_service(
        cart_repository,
        order_repository,
        product_repository,
        inventory_repository,
        voucher_repository);
    ReviewApplicationService review_service(order_repository, review_repository);
    ReturnsApplicationService returns_service(order_repository, return_repository);

    auto sign_in_result = auth_service.sign_in("client001", "hash:123456");
    if (!sign_in_result) {
        std::cerr << "failed to sign in demo account\n";
        return 1;
    }

    auto profile_result = customer_service.get_profile_by_account_id(sign_in_result.value().account.id());
    if (!profile_result) {
        std::cerr << "failed to load customer profile\n";
        return 1;
    }

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

    auto wishlist_result = customer_service.add_to_wishlist(customer_id, ProductId{"product-001"});
    if (!wishlist_result) {
        std::cerr << "failed to update wishlist\n";
        return 1;
    }

    auto preview_result = order_service.preview_checkout(cart_id, std::string("WELCOME10"));
    if (!preview_result) {
        std::cerr << "failed to preview checkout\n";
        return 1;
    }

    auto order_result = order_service.place_order(PlaceOrderCommand{
        OrderId{"order-001"},
        cart_id,
        customer_id,
        ShippingAddress{
            profile_result.value().full_name,
            profile_result.value().phone,
            "12 Nguyen Hue",
            "",
            "Ben Nghe",
            "District 1",
            profile_result.value().city,
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

    auto packed_order = order_service.advance_order_status(OrderId{"order-001"}, OrderStatus::Packed);
    auto shipped_order = order_service.advance_order_status(OrderId{"order-001"}, OrderStatus::Shipped);
    auto delivered_order = order_service.advance_order_status(OrderId{"order-001"}, OrderStatus::Delivered);
    auto completed_order = order_service.advance_order_status(OrderId{"order-001"}, OrderStatus::Completed);
    if (!packed_order || !shipped_order || !delivered_order || !completed_order) {
        std::cerr << "failed to complete order lifecycle\n";
        return 1;
    }

    const auto customer_orders = order_service.get_customer_orders(customer_id);
    if (customer_orders.empty()) {
        std::cerr << "failed to read customer order history\n";
        return 1;
    }

    const auto& saved_order = customer_orders.front();
    auto review_result = review_service.create_review(
        saved_order.id(),
        ReviewId{"review-001"},
        customer_id,
        ProductId{"product-001"},
        std::optional<VariantId>{VariantId{"variant-001-black-m"}},
        5,
        "Excellent fit and finish.");
    if (!review_result) {
        std::cerr << "failed to create product review\n";
        return 1;
    }

    auto return_result = returns_service.create_return_request(
        saved_order.id(),
        ReturnId{"return-001"},
        saved_order.items().front().id,
        1,
        "Size exchange");
    if (!return_result) {
        std::cerr << "failed to create return request\n";
        return 1;
    }

    const auto& receipt = order_result.value();
    const auto& final_order = completed_order.value();
    const auto product_reviews = review_service.get_product_reviews(ProductId{"product-001"});
    const auto order_returns = returns_service.get_order_returns(saved_order.id());
    std::cout << "order_id=" << receipt.order.id().value << '\n';
    std::cout << "customer=" << profile_result.value().full_name << '\n';
    std::cout << "items=" << receipt.order.items().size() << '\n';
    std::cout << "preview_total=" << preview_result.value().total.minor_units() << '\n';
    std::cout << "subtotal=" << receipt.subtotal.minor_units() << '\n';
    std::cout << "discount=" << receipt.discount.minor_units() << '\n';
    std::cout << "total=" << receipt.total.minor_units() << '\n';
    std::cout << "status=" << static_cast<int>(final_order.status()) << '\n';
    std::cout << "reviews=" << product_reviews.size() << '\n';
    std::cout << "returns=" << order_returns.size() << '\n';

    return 0;
}
