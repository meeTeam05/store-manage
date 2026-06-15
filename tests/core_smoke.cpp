#include <cassert>
#include <chrono>
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
    customer_repository.save(Customer(
        CustomerId{"customer-001"},
        AccountId{"account-001"},
        "Client Name",
        "0900000000",
        ShippingAddress{"Client Name", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"}));
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
        Money::from_minor(1000000),
        Money::from_minor(800000),
        now - std::chrono::hours(24),
        now + std::chrono::hours(24),
        10,
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

    const auto customer_id = CustomerId{"customer-001"};
    const auto cart_id = CartId{"cart-001"};

    auto sign_in = auth_service.sign_in("client001", "hash:123456");
    assert(sign_in);

    auto profile = customer_service.get_profile(customer_id);
    assert(profile);
    auto account_profile = customer_service.get_profile_by_account_id(AccountId{"account-001"});
    assert(account_profile);

    auto add_to_cart = cart_service.add_item(cart_id, customer_id, VariantId{"variant-001"}, 2);
    assert(add_to_cart);
    assert(add_to_cart.value().subtotal() == Money::from_minor(9800000));

    auto change_quantity = cart_service.change_quantity(cart_id, VariantId{"variant-001"}, 1);
    assert(change_quantity);
    assert(change_quantity.value().subtotal() == Money::from_minor(4900000));

    auto add_again = cart_service.add_item(cart_id, customer_id, VariantId{"variant-001"}, 1);
    assert(add_again);
    assert(add_again.value().subtotal() == Money::from_minor(9800000));

    auto preview = order_service.preview_checkout(cart_id, std::string("WELCOME10"));
    assert(preview);
    assert(preview.value().total == Money::from_minor(9000000));

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

    const auto customer_orders = order_service.get_customer_orders(customer_id);
    assert(customer_orders.size() == 1);

    auto review = review_service.create_review(
        OrderId{"order-001"},
        ReviewId{"review-001"},
        customer_id,
        ProductId{"product-001"},
        std::optional<VariantId>{VariantId{"variant-001"}},
        5,
        "Excellent structure.");
    assert(review);
    assert(review_service.get_product_reviews(ProductId{"product-001"}).size() == 1);
    assert(review_service.get_customer_reviews(customer_id).size() == 1);

    auto return_request = returns_service.create_return_request(
        OrderId{"order-001"},
        ReturnId{"return-001"},
        completed.value().items().front().id,
        1,
        "Need another size");
    assert(return_request);
    assert(returns_service.get_order_returns(OrderId{"order-001"}).size() == 1);

    return 0;
}
