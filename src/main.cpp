#include <chrono>
#include <iostream>
#include <optional>

#include "application/cart/cart_application_service.hpp"
#include "application/catalog/catalog_application_service.hpp"
#include "application/customer/customer_application_service.hpp"
#include "application/identity/auth_application_service.hpp"
#include "application/order/order_application_service.hpp"
#include "application/payment/payment_application_service.hpp"
#include "application/report/report_application_service.hpp"
#include "application/returns/returns_application_service.hpp"
#include "application/review/review_application_service.hpp"
#include "application/shipping/shipping_application_service.hpp"
#include "application/staff/return_management_service.hpp"
#include "application/staff/store_management_service.hpp"
#include "domain/catalog/product.hpp"
#include "domain/customer/customer.hpp"
#include "domain/identity/account.hpp"
#include "domain/inventory/inventory_item.hpp"
#include "domain/order/order.hpp"
#include "domain/pricing/voucher.hpp"
#include "infrastructure/persistence/in_memory/in_memory_repositories.hpp"
#include "server.hpp"

int main() {
    using namespace fashion_store::application::cart;
    using namespace fashion_store::application::catalog;
    using namespace fashion_store::application::customer;
    using namespace fashion_store::application::identity;
    using namespace fashion_store::application::order;
    using namespace fashion_store::application::payment;
    using namespace fashion_store::application::report;
    using namespace fashion_store::application::returns;
    using namespace fashion_store::application::review;
    using namespace fashion_store::application::shipping;
    using namespace fashion_store::application::staff;
    using namespace fashion_store::domain::catalog;
    using namespace fashion_store::domain::customer;
    using namespace fashion_store::domain::identity;
    using namespace fashion_store::domain::inventory;
    using namespace fashion_store::domain::order;
    using namespace fashion_store::domain::pricing;
    using namespace fashion_store::domain::shared;
    using namespace fashion_store::infrastructure::persistence::in_memory;

    // ── Repositories ──────────────────────────────────────────────────────────
    InMemoryProductRepository   product_repository;
    InMemoryCustomerRepository  customer_repository;
    InMemoryAccountRepository   account_repository;
    InMemoryInventoryRepository inventory_repository;
    InMemoryCartRepository      cart_repository;
    InMemoryOrderRepository     order_repository;
    InMemoryVoucherRepository   voucher_repository;
    InMemoryReviewRepository    review_repository;
    InMemoryReturnRepository    return_repository;

    // ── Products ──────────────────────────────────────────────────────────────
    {
        Product p(ProductId{"product-001"}, "Structured House Overshirt",
                  Category::Outerwear, "Relaxed luxury overshirt for daily wear.", "Resort 2026");
        (void)p.add_variant(ProductVariantDraft{
            VariantId{"variant-001-black-m"}, "MA-OVR-001-BLK-M",
            Size{"M"}, Color{"Black"}, Money::from_minor(490000)});
        (void)p.add_variant(ProductVariantDraft{
            VariantId{"variant-001-white-l"}, "MA-OVR-001-WHT-L",
            Size{"L"}, Color{"White"}, Money::from_minor(490000)});
        product_repository.save(p);
    }
    {
        Product p(ProductId{"product-002"}, "Silk Slip Dress",
                  Category::Dresses, "Effortless silk slip for all occasions.", "Summer 2026");
        (void)p.add_variant(ProductVariantDraft{
            VariantId{"variant-002-ivory-s"}, "MA-DRS-002-IVR-S",
            Size{"S"}, Color{"Ivory"}, Money::from_minor(750000)});
        (void)p.add_variant(ProductVariantDraft{
            VariantId{"variant-002-navy-m"}, "MA-DRS-002-NVY-M",
            Size{"M"}, Color{"Navy"}, Money::from_minor(750000)});
        product_repository.save(p);
    }
    {
        Product p(ProductId{"product-003"}, "Relaxed Chino",
                  Category::Bottoms, "Everyday-ready relaxed chino.", "Everyday Essentials");
        (void)p.add_variant(ProductVariantDraft{
            VariantId{"variant-003-khaki-m"}, "MA-BTM-003-KHK-M",
            Size{"M"}, Color{"Khaki"}, Money::from_minor(320000)});
        (void)p.add_variant(ProductVariantDraft{
            VariantId{"variant-003-olive-l"}, "MA-BTM-003-OLV-L",
            Size{"L"}, Color{"Olive"}, Money::from_minor(320000)});
        product_repository.save(p);
    }

    // ── Inventory ─────────────────────────────────────────────────────────────
    inventory_repository.save(InventoryItem(VariantId{"variant-001-black-m"}, 20));
    inventory_repository.save(InventoryItem(VariantId{"variant-001-white-l"}, 15));
    inventory_repository.save(InventoryItem(VariantId{"variant-002-ivory-s"}, 10));
    inventory_repository.save(InventoryItem(VariantId{"variant-002-navy-m"}, 10));
    inventory_repository.save(InventoryItem(VariantId{"variant-003-khaki-m"}, 25));
    inventory_repository.save(InventoryItem(VariantId{"variant-003-olive-l"}, 25));

    // ── Accounts and Customers ────────────────────────────────────────────────
    const ShippingAddress default_addr{
        "Nguyen Van A", "0900000000",
        "12 Nguyen Hue", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"};

    customer_repository.save(Customer(
        CustomerId{"customer-001"}, AccountId{"account-001"},
        "Nguyen Van A", "0900000000", default_addr));

    account_repository.save(Account(
        AccountId{"account-001"}, "client001", "hash:123456",
        Role::Customer, AccountStatus::Active));
    account_repository.save(Account(
        AccountId{"account-002"}, "staff001", "hash:123456",
        Role::Staff, AccountStatus::Active));
    account_repository.save(Account(
        AccountId{"account-003"}, "admin001", "hash:123456",
        Role::Admin, AccountStatus::Active));

    // ── Voucher ────────────────────────────────────────────────────────────────
    const auto now = std::chrono::system_clock::now();
    voucher_repository.save(Voucher(
        "WELCOME10", DiscountType::Percentage, 10,
        Money::from_minor(300000), Money::from_minor(80000),
        now - std::chrono::hours(24),
        now + std::chrono::hours(24 * 30),
        100, true));

    // ── Services ──────────────────────────────────────────────────────────────
    CartApplicationService      cart_svc(cart_repository, product_repository);
    CustomerApplicationService  customer_svc(customer_repository);
    AuthApplicationService      auth_svc(account_repository);
    CatalogApplicationService   catalog_svc(product_repository);
    OrderApplicationService     order_svc(
        cart_repository, order_repository,
        product_repository, inventory_repository, voucher_repository);
    ReviewApplicationService    review_svc(order_repository, review_repository);
    ReturnsApplicationService   returns_svc(order_repository, return_repository);
    ReturnManagementService     return_mgmt_svc(
        return_repository, order_repository, inventory_repository);
    StoreManagementService      store_mgmt_svc(
        product_repository, inventory_repository,
        order_repository, voucher_repository, order_svc);
    PaymentApplicationService   payment_svc;
    ShippingApplicationService  shipping_svc;
    ReportApplicationService    report_svc(order_repository, inventory_repository);

    // ── Seed: order-seed-001 (Completed, for review/return demo) ─────────────
    {
        const CartId cid{"cart-seed-001"};
        (void)cart_svc.add_item(cid, CustomerId{"customer-001"},
                                VariantId{"variant-001-black-m"}, 1);
        auto placed = order_svc.place_order(PlaceOrderCommand{
            OrderId{"order-seed-001"}, cid,
            CustomerId{"customer-001"}, default_addr,
            PaymentMethod::EWallet, std::string("WELCOME10")});
        if (!placed) { std::cerr << "seed order-seed-001 place failed\n"; return 1; }
        if (!order_svc.mark_order_paid(OrderId{"order-seed-001"})) {
            std::cerr << "seed order-seed-001 pay failed\n"; return 1;
        }
        (void)order_svc.advance_order_status(OrderId{"order-seed-001"}, OrderStatus::Packed);
        (void)order_svc.advance_order_status(OrderId{"order-seed-001"}, OrderStatus::Shipped);
        (void)order_svc.advance_order_status(OrderId{"order-seed-001"}, OrderStatus::Delivered);
        auto completed = order_svc.advance_order_status(
            OrderId{"order-seed-001"}, OrderStatus::Completed);
        if (!completed) { std::cerr << "seed order-seed-001 complete failed\n"; return 1; }
    }

    // ── Seed: order-seed-002 (Paid, for advance/cancel demo) ─────────────────
    {
        const CartId cid{"cart-seed-002"};
        (void)cart_svc.add_item(cid, CustomerId{"customer-001"},
                                VariantId{"variant-002-ivory-s"}, 1);
        auto placed = order_svc.place_order(PlaceOrderCommand{
            OrderId{"order-seed-002"}, cid,
            CustomerId{"customer-001"}, default_addr,
            PaymentMethod::Cash, std::nullopt});
        if (!placed) { std::cerr << "seed order-seed-002 place failed\n"; return 1; }
        if (!order_svc.mark_order_paid(OrderId{"order-seed-002"})) {
            std::cerr << "seed order-seed-002 pay failed\n"; return 1;
        }
    }

    (void)customer_svc;  // used only in sign-in handler via customer_repository directly

    // ── HTTP server ───────────────────────────────────────────────────────────
    httplib::Server svr;
    fashion_store::server::setup_server(
        svr,
        auth_svc, catalog_svc, cart_svc,
        customer_repository,
        order_svc, review_svc, returns_svc,
        return_mgmt_svc, store_mgmt_svc,
        payment_svc, shipping_svc, report_svc);

    std::cout << "listening on port 8080\n";
    std::cout.flush();
    svr.listen("0.0.0.0", 8080);
    return 0;
}
