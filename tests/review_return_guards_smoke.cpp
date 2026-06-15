#include <cassert>
#include <optional>

#include "application/review/review_application_service.hpp"
#include "application/returns/returns_application_service.hpp"
#include "domain/order/order.hpp"
#include "infrastructure/persistence/in_memory/in_memory_repositories.hpp"

namespace {

using namespace fashion_store::application::review;
using namespace fashion_store::application::returns;
using namespace fashion_store::domain::order;
using namespace fashion_store::domain::review;
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

void verify_duplicate_review_is_rejected() {
    InMemoryOrderRepository order_repository;
    InMemoryReviewRepository review_repository;

    const auto order_id = OrderId{"order-review-001"};
    const auto customer_id = CustomerId{"customer-review-001"};
    const auto product_id = ProductId{"product-review-001"};
    const auto variant_id = VariantId{"variant-review-001"};

    order_repository.save(build_completed_order(order_id, customer_id, product_id, variant_id, 1));

    ReviewApplicationService review_service(order_repository, review_repository);

    auto first_review = review_service.create_review(
        order_id,
        ReviewId{"review-001"},
        customer_id,
        product_id,
        std::optional<VariantId>{variant_id},
        5,
        "Excellent structure.");
    assert(first_review);

    auto duplicate_review = review_service.create_review(
        order_id,
        ReviewId{"review-002"},
        customer_id,
        product_id,
        std::optional<VariantId>{variant_id},
        4,
        "Second review should fail.");
    assert(!duplicate_review);
    assert(duplicate_review.error() == CreateReviewError::DuplicateReview);
    assert(review_repository.find_by_customer_id(customer_id).size() == 1);
}

void verify_cumulative_return_quantity_is_rejected() {
    InMemoryOrderRepository order_repository;
    InMemoryReturnRepository return_repository;

    const auto order_id = OrderId{"order-return-001"};
    const auto customer_id = CustomerId{"customer-return-001"};
    const auto product_id = ProductId{"product-return-001"};
    const auto variant_id = VariantId{"variant-return-001"};

    const auto order = build_completed_order(order_id, customer_id, product_id, variant_id, 2);
    order_repository.save(order);

    ReturnsApplicationService returns_service(order_repository, return_repository);

    const auto& order_item_id = order.items().front().id;
    auto first_request = returns_service.create_return_request(
        order_id,
        ReturnId{"return-001"},
        order_item_id,
        1,
        "Need another size");
    assert(first_request);

    auto second_request = returns_service.create_return_request(
        order_id,
        ReturnId{"return-002"},
        order_item_id,
        2,
        "Trying to exceed purchased quantity");
    assert(!second_request);
    assert(second_request.error() == CreateReturnError::QuantityExceededByExistingRequests);
    assert(return_repository.find_by_order_id(order_id).size() == 1);
}

}  // namespace

int main() {
    verify_duplicate_review_is_rejected();
    verify_cumulative_return_quantity_is_rejected();
    return 0;
}
