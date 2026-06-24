#include <chrono>
#include <filesystem>
#include <fstream>
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
#include "infrastructure/persistence/file/file_repositories.hpp"
#include "server.hpp"

namespace {

std::filesystem::path resolve_project_path(const std::string& leaf) {
    const auto cwd = std::filesystem::current_path();
    if (std::filesystem::exists(cwd / "src") && std::filesystem::exists(cwd / "web")) {
        return cwd / leaf;
    }

    const auto parent = cwd.parent_path();
    if (std::filesystem::exists(parent / "src") && std::filesystem::exists(parent / "web")) {
        return parent / leaf;
    }

    return cwd / leaf;
}

}  // namespace

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
    using namespace fashion_store::infrastructure::persistence::file;

    const auto data_dir = resolve_project_path("data");
    std::filesystem::create_directories(data_dir);
    const auto product_storefront_path = data_dir / "product_storefront.json";

    if (!std::filesystem::exists(product_storefront_path) ||
        std::filesystem::is_empty(product_storefront_path)) {
        std::ofstream storefront_seed(product_storefront_path, std::ios::trunc);
        storefront_seed
            << "{\n"
            << "  \"product-001\": [\n"
            << "    \"https://images.unsplash.com/photo-1529139574466-a303027c1d8b?auto=format&fit=crop&w=1200&q=80\",\n"
            << "    \"https://images.unsplash.com/photo-1483985988355-763728e1935b?auto=format&fit=crop&w=1200&q=80\"\n"
            << "  ]\n"
            << "}\n";
    }

    FileProductRepository product_repository(data_dir / "products.txt");
    FileCustomerRepository customer_repository(data_dir / "customers.txt");
    FileAccountRepository account_repository(data_dir / "accounts.txt");
    FileInventoryRepository inventory_repository(data_dir / "inventory.txt");
    FileCartRepository cart_repository(data_dir / "carts.txt");
    FileOrderRepository order_repository(data_dir / "orders.txt");
    FileVoucherRepository voucher_repository(data_dir / "vouchers.txt");
    FileReviewRepository review_repository(data_dir / "reviews.txt");
    FileReturnRepository return_repository(data_dir / "returns.txt");

    const ShippingAddress default_addr{
        "Nguyen Van A",
        "0900000000",
        "12 Nguyen Hue",
        "",
        "Ben Nghe",
        "District 1",
        "Ho Chi Minh City",
        "Vietnam"
    };

    if (!product_repository.find_by_id(ProductId{"product-001"}).has_value()) {
        Product product(
            ProductId{"product-001"},
            "Structured Coat",
            Category::Outerwear,
            "Long-line black coat with quiet shoulder structure, fluid drape, and a disciplined silhouette for day-to-evening wear.",
            "Maison Aureline 2026");
        (void)product.add_variant(ProductVariantDraft{
            VariantId{"variant-001-black-s"},
            "COAT-BLK-S",
            Size{"S"},
            Color{"Black"},
            Money::from_minor(4900000)});
        (void)product.add_variant(ProductVariantDraft{
            VariantId{"variant-001-black-m"},
            "COAT-BLK-M",
            Size{"M"},
            Color{"Black"},
            Money::from_minor(4900000)});
        (void)product.add_variant(ProductVariantDraft{
            VariantId{"variant-001-black-l"},
            "COAT-BLK-L",
            Size{"L"},
            Color{"Black"},
            Money::from_minor(4900000)});
        (void)product.add_variant(ProductVariantDraft{
            VariantId{"variant-001-ivory-s"},
            "COAT-IVR-S",
            Size{"S"},
            Color{"Ivory"},
            Money::from_minor(4900000)});
        (void)product.add_variant(ProductVariantDraft{
            VariantId{"variant-001-ivory-m"},
            "COAT-IVR-M",
            Size{"M"},
            Color{"Ivory"},
            Money::from_minor(4900000)});
        product_repository.save(product);
    }

    if (!product_repository.find_by_id(ProductId{"product-002"}).has_value()) {
        Product product(
            ProductId{"product-002"},
            "Ivory Column Dress",
            Category::Dresses,
            "Fluid ivory column dress with a draped cowl neck, open back, and floor-skimming hem for elevated evening wear.",
            "Maison Aureline 2026");
        (void)product.add_variant(ProductVariantDraft{
            VariantId{"variant-002-ivory-s"}, "DRESS-IVR-S", Size{"S"}, Color{"Ivory"}, Money::from_minor(5600000)});
        (void)product.add_variant(ProductVariantDraft{
            VariantId{"variant-002-ivory-m"}, "DRESS-IVR-M", Size{"M"}, Color{"Ivory"}, Money::from_minor(5600000)});
        (void)product.add_variant(ProductVariantDraft{
            VariantId{"variant-002-ivory-l"}, "DRESS-IVR-L", Size{"L"}, Color{"Ivory"}, Money::from_minor(5600000)});
        (void)product.add_variant(ProductVariantDraft{
            VariantId{"variant-002-beige-s"}, "DRESS-BEG-S", Size{"S"}, Color{"Beige"}, Money::from_minor(5600000)});
        (void)product.add_variant(ProductVariantDraft{
            VariantId{"variant-002-beige-m"}, "DRESS-BEG-M", Size{"M"}, Color{"Beige"}, Money::from_minor(5600000)});
        product_repository.save(product);
    }

    if (!product_repository.find_by_id(ProductId{"product-003"}).has_value()) {
        Product product(
            ProductId{"product-003"},
            "Tailored Blazer",
            Category::Outerwear,
            "Classic double-breasted tailored blazer in charcoal wool, features structured shoulders and a modern relaxed cut.",
            "Maison Aureline 2026");
        (void)product.add_variant(ProductVariantDraft{
            VariantId{"variant-003-charcoal-s"}, "BLZR-CHA-S", Size{"S"}, Color{"Charcoal"}, Money::from_minor(3800000)});
        (void)product.add_variant(ProductVariantDraft{
            VariantId{"variant-003-charcoal-m"}, "BLZR-CHA-M", Size{"M"}, Color{"Charcoal"}, Money::from_minor(3800000)});
        (void)product.add_variant(ProductVariantDraft{
            VariantId{"variant-003-charcoal-l"}, "BLZR-CHA-L", Size{"L"}, Color{"Charcoal"}, Money::from_minor(3800000)});
        (void)product.add_variant(ProductVariantDraft{
            VariantId{"variant-003-black-s"}, "BLZR-BLK-S", Size{"S"}, Color{"Black"}, Money::from_minor(3800000)});
        (void)product.add_variant(ProductVariantDraft{
            VariantId{"variant-003-black-m"}, "BLZR-BLK-M", Size{"M"}, Color{"Black"}, Money::from_minor(3800000)});
        product_repository.save(product);
    }

    if (!inventory_repository.find_by_variant_id(VariantId{"variant-001-black-s"}).has_value()) {
        inventory_repository.save(InventoryItem(VariantId{"variant-001-black-s"}, 10));
    }
    if (!inventory_repository.find_by_variant_id(VariantId{"variant-001-black-m"}).has_value()) {
        inventory_repository.save(InventoryItem(VariantId{"variant-001-black-m"}, 12));
    }
    if (!inventory_repository.find_by_variant_id(VariantId{"variant-001-black-l"}).has_value()) {
        inventory_repository.save(InventoryItem(VariantId{"variant-001-black-l"}, 8));
    }
    if (!inventory_repository.find_by_variant_id(VariantId{"variant-001-ivory-s"}).has_value()) {
        inventory_repository.save(InventoryItem(VariantId{"variant-001-ivory-s"}, 9));
    }
    if (!inventory_repository.find_by_variant_id(VariantId{"variant-001-ivory-m"}).has_value()) {
        inventory_repository.save(InventoryItem(VariantId{"variant-001-ivory-m"}, 9));
    }

    if (!inventory_repository.find_by_variant_id(VariantId{"variant-002-ivory-s"}).has_value()) {
        inventory_repository.save(InventoryItem(VariantId{"variant-002-ivory-s"}, 5));
    }
    if (!inventory_repository.find_by_variant_id(VariantId{"variant-002-ivory-m"}).has_value()) {
        inventory_repository.save(InventoryItem(VariantId{"variant-002-ivory-m"}, 4));
    }
    if (!inventory_repository.find_by_variant_id(VariantId{"variant-002-ivory-l"}).has_value()) {
        inventory_repository.save(InventoryItem(VariantId{"variant-002-ivory-l"}, 3));
    }
    if (!inventory_repository.find_by_variant_id(VariantId{"variant-002-beige-s"}).has_value()) {
        inventory_repository.save(InventoryItem(VariantId{"variant-002-beige-s"}, 6));
    }
    if (!inventory_repository.find_by_variant_id(VariantId{"variant-002-beige-m"}).has_value()) {
        inventory_repository.save(InventoryItem(VariantId{"variant-002-beige-m"}, 5));
    }

    if (!inventory_repository.find_by_variant_id(VariantId{"variant-003-charcoal-s"}).has_value()) {
        inventory_repository.save(InventoryItem(VariantId{"variant-003-charcoal-s"}, 7));
    }
    if (!inventory_repository.find_by_variant_id(VariantId{"variant-003-charcoal-m"}).has_value()) {
        inventory_repository.save(InventoryItem(VariantId{"variant-003-charcoal-m"}, 8));
    }
    if (!inventory_repository.find_by_variant_id(VariantId{"variant-003-charcoal-l"}).has_value()) {
        inventory_repository.save(InventoryItem(VariantId{"variant-003-charcoal-l"}, 6));
    }
    if (!inventory_repository.find_by_variant_id(VariantId{"variant-003-black-s"}).has_value()) {
        inventory_repository.save(InventoryItem(VariantId{"variant-003-black-s"}, 5));
    }
    if (!inventory_repository.find_by_variant_id(VariantId{"variant-003-black-m"}).has_value()) {
        inventory_repository.save(InventoryItem(VariantId{"variant-003-black-m"}, 6));
    }

    if (!customer_repository.find_by_id(CustomerId{"customer-001"}).has_value()) {
        customer_repository.save(Customer(
            CustomerId{"customer-001"},
            AccountId{"account-001"},
            "Nguyen Van A",
            "0900000000",
            default_addr));
    }

    if (!account_repository.find_by_id(AccountId{"account-001"}).has_value()) {
        account_repository.save(Account(
            AccountId{"account-001"},
            "client001",
            "hash:123456",
            Role::Customer,
            AccountStatus::Active));
    }
    if (!account_repository.find_by_id(AccountId{"account-002"}).has_value()) {
        account_repository.save(Account(
            AccountId{"account-002"},
            "staff001",
            "hash:staff123",
            Role::Staff,
            AccountStatus::Active));
    }
    if (!account_repository.find_by_id(AccountId{"account-003"}).has_value()) {
        account_repository.save(Account(
            AccountId{"account-003"},
            "admin001",
            "hash:admin123",
            Role::Admin,
            AccountStatus::Active));
    }

    const auto now = std::chrono::system_clock::now();
    if (!voucher_repository.find_by_code("WELCOME10").has_value()) {
        voucher_repository.save(Voucher(
            "WELCOME10",
            DiscountType::Percentage,
            10,
            Money::from_minor(1000000),
            Money::from_minor(800000),
            now - std::chrono::hours(24),
            now + std::chrono::hours(24 * 30),
            100,
            true));
    }

    CartApplicationService cart_svc(cart_repository, product_repository);
    CustomerApplicationService customer_svc(customer_repository);
    AuthApplicationService auth_svc(account_repository);
    CatalogApplicationService catalog_svc(product_repository);
    OrderApplicationService order_svc(
        cart_repository,
        order_repository,
        product_repository,
        inventory_repository,
        voucher_repository);
    ReviewApplicationService review_svc(order_repository, review_repository);
    ReturnsApplicationService returns_svc(order_repository, return_repository);
    ReturnManagementService return_mgmt_svc(
        return_repository,
        order_repository,
        inventory_repository);
    StoreManagementService store_mgmt_svc(
        product_repository,
        inventory_repository,
        order_repository,
        voucher_repository,
        order_svc);
    PaymentApplicationService payment_svc;
    ShippingApplicationService shipping_svc;
    ReportApplicationService report_svc(order_repository, inventory_repository);

    if (order_repository.list_all().empty()) {
        const CartId completed_cart_id{"cart-seed-001"};
        (void)cart_svc.add_item(
            completed_cart_id,
            CustomerId{"customer-001"},
            VariantId{"variant-001-black-m"},
            1);
        auto completed_seed = order_svc.place_order(PlaceOrderCommand{
            OrderId{"order-seed-001"},
            completed_cart_id,
            CustomerId{"customer-001"},
            default_addr,
            PaymentMethod::EWallet,
            std::string("WELCOME10")});
        if (!completed_seed) {
            std::cerr << "seed order-seed-001 place failed\n";
            return 1;
        }
        if (!order_svc.mark_order_paid(OrderId{"order-seed-001"})) {
            std::cerr << "seed order-seed-001 pay failed\n";
            return 1;
        }
        (void)order_svc.advance_order_status(OrderId{"order-seed-001"}, OrderStatus::Packed);
        (void)order_svc.advance_order_status(OrderId{"order-seed-001"}, OrderStatus::Shipped);
        (void)order_svc.advance_order_status(OrderId{"order-seed-001"}, OrderStatus::Delivered);
        if (!order_svc.advance_order_status(OrderId{"order-seed-001"}, OrderStatus::Completed)) {
            std::cerr << "seed order-seed-001 complete failed\n";
            return 1;
        }

        const CartId pending_cart_id{"cart-seed-002"};
        (void)cart_svc.add_item(
            pending_cart_id,
            CustomerId{"customer-001"},
            VariantId{"variant-001-ivory-s"},
            1);
        if (!order_svc.place_order(PlaceOrderCommand{
                OrderId{"order-seed-002"},
                pending_cart_id,
                CustomerId{"customer-001"},
                default_addr,
                PaymentMethod::BankTransfer,
                std::nullopt})) {
            std::cerr << "seed order-seed-002 place failed\n";
            return 1;
        }
    }

    (void)customer_svc;

    httplib::Server svr;
    fashion_store::server::setup_server(
        svr,
        auth_svc,
        catalog_svc,
        cart_svc,
        account_repository,
        customer_repository,
        order_svc,
        review_svc,
        returns_svc,
        return_mgmt_svc,
        store_mgmt_svc,
        payment_svc,
        shipping_svc,
        report_svc,
        product_storefront_path);

    std::cout << "listening on port 8080\n";
    std::cout.flush();
    svr.listen("0.0.0.0", 8080);
    return 0;
}
