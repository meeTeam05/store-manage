#include <cassert>

#include "application/returns/returns_application_service.hpp"
#include "application/staff/return_management_service.hpp"
#include "domain/inventory/inventory_item.hpp"
#include "domain/order/order.hpp"
#include "infrastructure/persistence/in_memory/in_memory_repositories.hpp"

namespace {

using namespace fashion_store::application::returns;
using namespace fashion_store::application::staff;
using namespace fashion_store::domain::inventory;
using namespace fashion_store::domain::order;
using namespace fashion_store::domain::returns;
using namespace fashion_store::domain::shared;
using namespace fashion_store::infrastructure::persistence::in_memory;

Order build_completed_order(const OrderId& order_id,
                            const CustomerId& customer_id,
                            const ProductId& product_id,
                            const VariantId& variant_id,
                            int quantity) {
    Order order(
        order_id,
        customer_id,
        {
            OrderItem{
                OrderItemId{order_id.value + "-item-001"},
                product_id,
                variant_id,
                "Structured Coat",
                "COAT-BLK-M",
                Size{"M"},
                Color{"Black"},
                Money::from_minor(4900000),
                quantity,
                Money::from_minor(0)}
        },
        ShippingAddress{"Client", "0900000000", "1 Le Loi", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"},
        PaymentMethod::EWallet);
    assert(order.submit_for_payment());
    assert(order.mark_paid());
    assert(order.mark_packed());
    assert(order.mark_shipped());
    assert(order.mark_delivered());
    assert(order.mark_completed());
    return order;
}

void verify_return_can_be_approved_restocked_refunded_and_closed() {
    InMemoryOrderRepository order_repository;
    InMemoryReturnRepository return_repository;
    InMemoryInventoryRepository inventory_repository;

    const auto order_id = OrderId{"order-return-management-001"};
    const auto customer_id = CustomerId{"customer-return-management-001"};
    const auto product_id = ProductId{"product-return-management-001"};
    const auto variant_id = VariantId{"variant-return-management-001"};
    auto order = build_completed_order(order_id, customer_id, product_id, variant_id, 2);
    order_repository.save(order);
    inventory_repository.save(InventoryItem(variant_id, 0));

    ReturnsApplicationService returns_service(order_repository, return_repository);
    auto request = returns_service.create_return_request(
        order_id,
        ReturnId{"return-management-001"},
        order.items().front().id,
        1,
        "Need another size");
    assert(request);

    ReturnManagementService management_service(return_repository, order_repository, inventory_repository);

    auto restock_before_approve = management_service.restock_return(ReturnId{"return-management-001"});
    assert(!restock_before_approve);
    assert(restock_before_approve.error() == ReturnManagementError::ReturnRuleViolation);

    auto approved = management_service.approve_return(ReturnId{"return-management-001"});
    assert(approved);
    assert(approved.value().status() == ReturnStatus::Approved);

    auto restocked = management_service.restock_return(ReturnId{"return-management-001"});
    assert(restocked);
    assert(restocked.value().status() == ReturnStatus::Restocked);

    auto inventory_after_restock = inventory_repository.find_by_variant_id(variant_id);
    assert(inventory_after_restock.has_value());
    assert(inventory_after_restock->on_hand() == 1);

    auto refunded = management_service.refund_return(ReturnId{"return-management-001"});
    assert(refunded);
    assert(refunded.value().status() == ReturnStatus::Refunded);

    auto closed = management_service.close_return(ReturnId{"return-management-001"});
    assert(closed);
    assert(closed.value().status() == ReturnStatus::Closed);

    auto stored = return_repository.find_by_id(ReturnId{"return-management-001"});
    assert(stored.has_value());
    assert(stored->status() == ReturnStatus::Closed);
    assert(return_repository.find_by_order_id(order_id).size() == 1);
}

void verify_rejected_return_can_be_closed_without_restock() {
    InMemoryOrderRepository order_repository;
    InMemoryReturnRepository return_repository;
    InMemoryInventoryRepository inventory_repository;

    const auto order_id = OrderId{"order-return-management-002"};
    const auto customer_id = CustomerId{"customer-return-management-002"};
    const auto product_id = ProductId{"product-return-management-002"};
    const auto variant_id = VariantId{"variant-return-management-002"};
    auto order = build_completed_order(order_id, customer_id, product_id, variant_id, 1);
    order_repository.save(order);
    inventory_repository.save(InventoryItem(variant_id, 0));

    ReturnsApplicationService returns_service(order_repository, return_repository);
    auto request = returns_service.create_return_request(
        order_id,
        ReturnId{"return-management-002"},
        order.items().front().id,
        1,
        "Return reason rejected by staff");
    assert(request);

    ReturnManagementService management_service(return_repository, order_repository, inventory_repository);
    auto rejected = management_service.reject_return(ReturnId{"return-management-002"});
    assert(rejected);
    assert(rejected.value().status() == ReturnStatus::Rejected);

    auto restock_rejected = management_service.restock_return(ReturnId{"return-management-002"});
    assert(!restock_rejected);
    assert(restock_rejected.error() == ReturnManagementError::ReturnRuleViolation);

    auto closed = management_service.close_return(ReturnId{"return-management-002"});
    assert(closed);
    assert(closed.value().status() == ReturnStatus::Closed);

    auto inventory_after_close = inventory_repository.find_by_variant_id(variant_id);
    assert(inventory_after_close.has_value());
    assert(inventory_after_close->on_hand() == 0);
}

}  // namespace

int main() {
    verify_return_can_be_approved_restocked_refunded_and_closed();
    verify_rejected_return_can_be_closed_without_restock();
    return 0;
}
