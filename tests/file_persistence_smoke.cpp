#include <cassert>
#include <chrono>
#include <filesystem>
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
#include "infrastructure/persistence/file/file_repositories.hpp"

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
    using namespace fashion_store::infrastructure::persistence::file;

    const auto base_dir = std::filesystem::temp_directory_path() / "fashion_store_file_persistence_smoke";
    std::filesystem::remove_all(base_dir);
    std::filesystem::create_directories(base_dir);

    FileProductRepository product_repository(base_dir / "products.txt");
    FileInventoryRepository inventory_repository(base_dir / "inventory.txt");
    FileCartRepository cart_repository(base_dir / "carts.txt");
    FileOrderRepository order_repository(base_dir / "orders.txt");
    FileVoucherRepository voucher_repository(base_dir / "vouchers.txt");
    FileAccountRepository account_repository(base_dir / "accounts.txt");
    FileCustomerRepository customer_repository(base_dir / "customers.txt");
    FileReviewRepository review_repository(base_dir / "reviews.txt");
    FileReturnRepository return_repository(base_dir / "returns.txt");

    Product product(
        ProductId{"product-001"},
        "Structured Coat",
        Category::Outerwear,
        "Luxury outerwear",
        "Resort 2026");
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

    account_repository.save(Account(
        AccountId{"account-001"},
        "client001",
        "hash:123456",
        Role::Customer,
        AccountStatus::Active));

    customer_repository.save(Customer(
        CustomerId{"customer-001"},
        AccountId{"account-001"},
        "Client Name",
        "0900000000",
        ShippingAddress{"Client Name", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"}));

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

    auto sign_in = auth_service.sign_in("client001", "hash:123456");
    assert(sign_in);

    auto profile = customer_service.get_profile_by_account_id(AccountId{"account-001"});
    assert(profile);

    auto add_to_cart = cart_service.add_item(CartId{"cart-001"}, CustomerId{"customer-001"}, VariantId{"variant-001"}, 2);
    assert(add_to_cart);

    auto place_order = order_service.place_order(PlaceOrderCommand{
        OrderId{"order-001"},
        CartId{"cart-001"},
        CustomerId{"customer-001"},
        ShippingAddress{"Client Name", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"},
        PaymentMethod::EWallet,
        std::string("WELCOME10")
    });
    assert(place_order);

    auto paid = order_service.mark_order_paid(OrderId{"order-001"});
    assert(paid);
    assert(order_service.advance_order_status(OrderId{"order-001"}, OrderStatus::Packed));
    assert(order_service.advance_order_status(OrderId{"order-001"}, OrderStatus::Shipped));
    assert(order_service.advance_order_status(OrderId{"order-001"}, OrderStatus::Delivered));
    auto completed = order_service.advance_order_status(OrderId{"order-001"}, OrderStatus::Completed);
    assert(completed);

    auto review = review_service.create_review(
        OrderId{"order-001"},
        ReviewId{"review-001"},
        CustomerId{"customer-001"},
        ProductId{"product-001"},
        std::optional<VariantId>{VariantId{"variant-001"}},
        5,
        "Excellent structure.");
    assert(review);

    auto request = returns_service.create_return_request(
        OrderId{"order-001"},
        ReturnId{"return-001"},
        completed.value().items().front().id,
        1,
        "Need another size");
    assert(request);

    assert(product_repository.list_all().size() == 1);
    assert(order_repository.find_by_customer_id(CustomerId{"customer-001"}).size() == 1);
    assert(review_repository.find_by_product_id(ProductId{"product-001"}).size() == 1);
    assert(return_repository.find_by_order_id(OrderId{"order-001"}).size() == 1);

    std::filesystem::remove_all(base_dir);
    return 0;
}
