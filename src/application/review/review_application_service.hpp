#pragma once

#include <optional>
#include <string>
#include <vector>

#include "domain/order/order_repository.hpp"
#include "domain/review/review.hpp"
#include "domain/review/review_policy.hpp"

namespace fashion_store::application::review {

using fashion_store::domain::order::IOrderRepository;
using fashion_store::domain::review::IReviewRepository;
using fashion_store::domain::review::Review;
using fashion_store::domain::review::ReviewEligibilityError;
using fashion_store::domain::review::ReviewPolicy;
using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::OrderId;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::Result;
using fashion_store::domain::shared::ReviewId;
using fashion_store::domain::shared::VariantId;

enum class CreateReviewError {
    OrderNotFound,
    ReviewNotEligible
};

class ReviewApplicationService {
public:
    ReviewApplicationService(IOrderRepository& order_repository, IReviewRepository& review_repository)
        : order_repository_(order_repository), review_repository_(review_repository) {}

    Result<Review, CreateReviewError> create_review(const OrderId& order_id,
                                                    ReviewId review_id,
                                                    const CustomerId& customer_id,
                                                    const ProductId& product_id,
                                                    std::optional<VariantId> variant_id,
                                                    int rating,
                                                    std::string comment) {
        auto order = order_repository_.find_by_id(order_id);
        if (!order.has_value()) {
            return Result<Review, CreateReviewError>::fail(CreateReviewError::OrderNotFound);
        }

        auto review = ReviewPolicy::create_review_for_order(
            std::move(review_id),
            *order,
            customer_id,
            product_id,
            std::move(variant_id),
            rating,
            std::move(comment));
        if (!review) {
            return Result<Review, CreateReviewError>::fail(CreateReviewError::ReviewNotEligible);
        }

        review_repository_.save(review.value());
        return Result<Review, CreateReviewError>::ok(review.value());
    }

    std::vector<Review> get_product_reviews(const ProductId& product_id) const {
        return review_repository_.find_by_product_id(product_id);
    }

    std::vector<Review> get_customer_reviews(const CustomerId& customer_id) const {
        return review_repository_.find_by_customer_id(customer_id);
    }

private:
    IOrderRepository& order_repository_;
    IReviewRepository& review_repository_;
};

}  // namespace fashion_store::application::review
