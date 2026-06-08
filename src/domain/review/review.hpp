#pragma once

#include <optional>
#include <string>

#include "domain/shared/types.hpp"

namespace fashion_store::domain::review {

using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::Result;
using fashion_store::domain::shared::ReviewId;
using fashion_store::domain::shared::VariantId;

enum class ReviewError {
    InvalidRating,
    PurchaseNotVerified
};

class Review {
public:
    static Result<Review, ReviewError> create(ReviewId review_id,
                                              CustomerId customer_id,
                                              ProductId product_id,
                                              std::optional<VariantId> variant_id,
                                              int rating,
                                              std::string comment,
                                              bool verified_purchase) {
        if (rating < 1 || rating > 5) {
            return Result<Review, ReviewError>::fail(ReviewError::InvalidRating);
        }
        if (!verified_purchase) {
            return Result<Review, ReviewError>::fail(ReviewError::PurchaseNotVerified);
        }
        return Result<Review, ReviewError>::ok(
            Review(std::move(review_id),
                   std::move(customer_id),
                   std::move(product_id),
                   std::move(variant_id),
                   rating,
                   std::move(comment),
                   verified_purchase));
    }

private:
    Review(ReviewId review_id,
           CustomerId customer_id,
           ProductId product_id,
           std::optional<VariantId> variant_id,
           int rating,
           std::string comment,
           bool verified_purchase)
        : review_id_(std::move(review_id)),
          customer_id_(std::move(customer_id)),
          product_id_(std::move(product_id)),
          variant_id_(std::move(variant_id)),
          rating_(rating),
          comment_(std::move(comment)),
          verified_purchase_(verified_purchase) {}

    ReviewId review_id_;
    CustomerId customer_id_;
    ProductId product_id_;
    std::optional<VariantId> variant_id_;
    int rating_;
    std::string comment_;
    bool verified_purchase_;
};

class IReviewRepository {
public:
    virtual ~IReviewRepository() = default;

    virtual void save(const Review& review) = 0;
};

}  // namespace fashion_store::domain::review
