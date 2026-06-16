#include <cassert>
#include <chrono>
#include <optional>

#include "api/fashion_store_api.hpp"
#include "application/cart/cart_application_service.hpp"
#include "application/catalog/catalog_application_service.hpp"
#include "application/customer/customer_application_service.hpp"
#include "application/identity/auth_application_service.hpp"
#include "application/order/order_application_service.hpp"
#include "domain/catalog/product.hpp"
#include "domain/customer/customer.hpp"
#include "domain/identity/account.hpp"
#include "domain/inventory/inventory_item.hpp"
#include "domain/pricing/voucher.hpp"
#include "infrastructure/persistence/in_memory/in_memory_repositories.hpp"

int main() {
    using namespace fashion_store::api;
    using namespace fashion_store::application::cart;
    using namespace fashion_store::application::catalog;
    using namespace fashion_store::application::customer;
    using namespace fashion_store::application::identity;
    using namespace fashion_store::application::order;
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

    Product product(ProductId{"product-api-001"}, "Structured Coat", Category::Outerwear, "Black wool coat", "Resort 2026");
    assert(product.add_variant(ProductVariantDraft{
        VariantId{"variant-api-001"},
        "COAT-BLK-M",
        Size{"M"},
        Color{"Black"},
        Money::from_minor(4900000)}));
    product_repository.save(product);
    inventory_repository.save(InventoryItem(VariantId{"variant-api-001"}, 3));
    account_repository.save(Account(AccountId{"account-api-001"}, "client-api", "hash:123456", Role::Customer, AccountStatus::Active));
    customer_repository.save(Customer(
        CustomerId{"customer-api-001"},
        AccountId{"account-api-001"},
        "Client API",
        "0900000000",
        ShippingAddress{"Client API", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"}));

    const auto now = std::chrono::system_clock::now();
    voucher_repository.save(Voucher(
        "API10",
        DiscountType::Percentage,
        10,
        Money::from_minor(1000000),
        Money::from_minor(500000),
        now - std::chrono::hours(1),
        now + std::chrono::hours(24),
        2,
        true));

    AuthApplicationService auth_service(account_repository);
    CatalogApplicationService catalog_service(product_repository);
    CartApplicationService cart_service(cart_repository, product_repository);
    CustomerApplicationService customer_service(customer_repository);
    OrderApplicationService order_service(cart_repository, order_repository, product_repository, inventory_repository, voucher_repository);
    FashionStoreApi api(auth_service, catalog_service, cart_service, customer_service, order_service);

    auto sign_in = api.sign_in("client-api", "hash:123456");
    assert(sign_in);
    assert(sign_in.value().account_id == "account-api-001");

    auto products = api.search_products(ProductSearchQuery{std::optional<std::string>{"coat"}});
    assert(products.size() == 1);
    assert(products.front().variant_count == 1);

    auto cart = api.add_to_cart(CartId{"cart-api-001"}, CustomerId{"customer-api-001"}, VariantId{"variant-api-001"}, 1);
    assert(cart);
    assert(cart.value().subtotal_minor == 4900000);

    auto checkout = api.checkout(PlaceOrderCommand{
        OrderId{"order-api-001"},
        CartId{"cart-api-001"},
        CustomerId{"customer-api-001"},
        ShippingAddress{"Client API", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"},
        PaymentMethod::EWallet,
        std::string("API10")});
    assert(checkout);
    assert(checkout.value().total_minor == 4410000);

    return 0;
}
