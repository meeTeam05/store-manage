#pragma once

#include <optional>

#include "domain/order/order.hpp"
#include "domain/review/review.hpp"

namespace fashion_store::domain::review {

using fashion_store::domain::order::Order;
using fashion_store::domain::order::OrderStatus;
using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::Result;
using fashion_store::domain::shared::ReviewId;
using fashion_store::domain::shared::VariantId;

enum class ReviewEligibilityError {
    CustomerMismatch,
    OrderNotCompleted,
    ProductNotPurchased,
    InvalidReviewContent
};

class ReviewPolicy {
public:
    static Result<Review, ReviewEligibilityError> create_review_for_order(
        ReviewId review_id,
        const Order& order,
        const CustomerId& customer_id,
        const ProductId& product_id,
        std::optional<VariantId> variant_id,
        int rating,
        std::string comment) {
        if (order.customer_id() != customer_id) {
            return Result<Review, ReviewEligibilityError>::fail(ReviewEligibilityError::CustomerMismatch);
        }
        if (order.status() != OrderStatus::Delivered &&
            order.status() != OrderStatus::Completed) {
            return Result<Review, ReviewEligibilityError>::fail(ReviewEligibilityError::OrderNotCompleted);
        }

        bool purchased = false;
        for (const auto& item : order.items()) {
            if (item.product_id == product_id &&
                (!variant_id.has_value() || item.variant_id == *variant_id)) {
                purchased = true;
                break;
            }
        }
        if (!purchased) {
            return Result<Review, ReviewEligibilityError>::fail(ReviewEligibilityError::ProductNotPurchased);
        }

        auto review = Review::create(
            std::move(review_id),
            customer_id,
            product_id,
            std::move(variant_id),
            rating,
            std::move(comment),
            true);
        if (!review) {
            return Result<Review, ReviewEligibilityError>::fail(ReviewEligibilityError::InvalidReviewContent);
        }
        return Result<Review, ReviewEligibilityError>::ok(review.value());
    }
};

}  // namespace fashion_store::domain::review
