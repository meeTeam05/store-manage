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
#include "infrastructure/demo_loader.hpp"
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
    const auto runtime_dir = data_dir / "runtime";
    std::filesystem::create_directories(runtime_dir);
    const auto product_storefront_path = runtime_dir / "product_storefront.json";

    FileProductRepository product_repository(runtime_dir / "products.txt");
    FileCustomerRepository customer_repository(runtime_dir / "customers.txt");
    FileAccountRepository account_repository(runtime_dir / "accounts.txt");
    FileInventoryRepository inventory_repository(runtime_dir / "inventory.txt");
    FileCartRepository cart_repository(runtime_dir / "carts.txt");
    FileOrderRepository order_repository(runtime_dir / "orders.txt");
    FileVoucherRepository voucher_repository(runtime_dir / "vouchers.txt");
    FileReviewRepository review_repository(runtime_dir / "reviews.txt");
    FileReturnRepository return_repository(runtime_dir / "returns.txt");

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

    const auto demo_dir = data_dir / "demo";
    fashion_store::infrastructure::initialize_demo_state(
        product_repository,
        inventory_repository,
        customer_repository,
        account_repository,
        voucher_repository,
        order_repository,
        cart_svc,
        order_svc,
        demo_dir,
        product_storefront_path
    );

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
