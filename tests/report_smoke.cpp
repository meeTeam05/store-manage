#include <cassert>
#include <optional>
#include <string>
#include <utility>

#include "application/report/report_application_service.hpp"
#include "domain/inventory/inventory_item.hpp"
#include "domain/order/order.hpp"
#include "infrastructure/persistence/in_memory/in_memory_repositories.hpp"

namespace {

using namespace fashion_store::application::report;
using namespace fashion_store::domain::inventory;
using namespace fashion_store::domain::order;
using namespace fashion_store::domain::shared;
using namespace fashion_store::infrastructure::persistence::in_memory;

Order build_order(OrderId order_id,
                  OrderStatus status,
                  PaymentStatus payment_status,
                  ProductId product_id,
                  VariantId variant_id,
                  std::string product_name,
                  int quantity,
                  Money unit_price,
                  Money discount_total) {
    return Order::rehydrate(
        std::move(order_id),
        CustomerId{"customer-report-001"},
        {
            OrderItem{
                OrderItemId{"item-" + variant_id.value},
                product_id,
                variant_id,
                std::move(product_name),
                "SKU-REPORT",
                Size{"M"},
                Color{"Black"},
                unit_price,
                quantity,
                Money::from_minor(0)}
        },
        ShippingAddress{"Client", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"},
        PaymentMethod::EWallet,
        status,
        payment_status,
        std::nullopt,
        discount_total);
}

void verify_report_service() {
    InMemoryOrderRepository order_repository;
    InMemoryInventoryRepository inventory_repository;

    order_repository.save(build_order(
        OrderId{"order-report-001"},
        OrderStatus::Completed,
        PaymentStatus::Paid,
        ProductId{"product-a"},
        VariantId{"variant-a"},
        "Structured Coat",
        2,
        Money::from_minor(1000),
        Money::from_minor(100)));

    order_repository.save(build_order(
        OrderId{"order-report-002"},
        OrderStatus::Shipped,
        PaymentStatus::Paid,
        ProductId{"product-b"},
        VariantId{"variant-b"},
        "Column Dress",
        3,
        Money::from_minor(500),
        Money::from_minor(0)));

    order_repository.save(build_order(
        OrderId{"order-report-003"},
        OrderStatus::Cancelled,
        PaymentStatus::Paid,
        ProductId{"product-c"},
        VariantId{"variant-c"},
        "Cancelled Item",
        10,
        Money::from_minor(9999),
        Money::from_minor(0)));

    inventory_repository.save(InventoryItem(VariantId{"variant-low"}, 2, 1));
    inventory_repository.save(InventoryItem(VariantId{"variant-empty"}, 0, 0));
    inventory_repository.save(InventoryItem(VariantId{"variant-ok"}, 12, 0));

    ReportApplicationService report_service(order_repository, inventory_repository);

    const auto revenue = report_service.build_revenue_report();
    assert(revenue.order_count == 2);
    assert(revenue.item_count == 5);
    assert(revenue.total_revenue == Money::from_minor(3400));

    const auto best_sellers = report_service.build_best_selling_products();
    assert(best_sellers.size() == 2);
    assert(best_sellers[0].product_id == ProductId{"product-b"});
    assert(best_sellers[0].quantity_sold == 3);
    assert(best_sellers[1].product_id == ProductId{"product-a"});
    assert(best_sellers[1].gross_sales == Money::from_minor(2000));

    const auto low_stock = report_service.build_low_stock_report(2);
    assert(low_stock.size() == 2);
    assert(low_stock[0].variant_id == VariantId{"variant-empty"});
    assert(low_stock[0].available == 0);
    assert(low_stock[1].variant_id == VariantId{"variant-low"});
    assert(low_stock[1].available == 1);
}

}  // namespace

int main() {
    verify_report_service();
    return 0;
}
