#pragma once

#include <string>
#include <vector>

#include "domain/shared/types.hpp"

namespace fashion_store::domain::catalog {

using fashion_store::domain::shared::Color;
using fashion_store::domain::shared::Money;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::Size;
using fashion_store::domain::shared::Status;
using fashion_store::domain::shared::VariantId;
using fashion_store::domain::shared::ok_status;
using fashion_store::domain::shared::require;

enum class Category {
    Tops,
    Bottoms,
    Outerwear,
    Dresses,
    Shoes,
    Accessories
};

enum class ProductStatus {
    Draft,
    Active,
    Discontinued
};

enum class ProductError {
    InvalidPrice,
    DuplicateVariantId,
    DuplicateSku,
    DuplicateOptionPair,
    VariantNotFound
};

struct ProductVariant {
    VariantId id;
    std::string sku;
    Size size;
    Color color;
    Money price;
    bool active{true};
};

struct ProductVariantDraft {
    VariantId id;
    std::string sku;
    Size size;
    Color color;
    Money price;
};

class Product {
public:
    Product(ProductId id,
            std::string name,
            Category category,
            std::string description,
            std::string collection = {},
            ProductStatus status = ProductStatus::Active)
        : id_(std::move(id)),
          name_(std::move(name)),
          category_(category),
          description_(std::move(description)),
          collection_(std::move(collection)),
          status_(status) {
        require(!id_.value.empty(), "product id must not be empty");
        require(!name_.empty(), "product name must not be empty");
    }

    const ProductId& id() const noexcept { return id_; }
    const std::string& name() const noexcept { return name_; }
    const std::vector<ProductVariant>& variants() const noexcept { return variants_; }

    Status<ProductError> add_variant(const ProductVariantDraft& draft) {
        if (draft.price < Money::from_minor(0)) {
            return Status<ProductError>::fail(ProductError::InvalidPrice);
        }
        if (find_variant(draft.id) != nullptr) {
            return Status<ProductError>::fail(ProductError::DuplicateVariantId);
        }
        for (const auto& variant : variants_) {
            if (variant.sku == draft.sku) {
                return Status<ProductError>::fail(ProductError::DuplicateSku);
            }
            if (variant.size == draft.size && variant.color == draft.color) {
                return Status<ProductError>::fail(ProductError::DuplicateOptionPair);
            }
        }
        variants_.push_back(ProductVariant{
            draft.id,
            draft.sku,
            draft.size,
            draft.color,
            draft.price,
            true
        });
        return ok_status<ProductError>();
    }

    const ProductVariant* find_variant(const VariantId& variant_id) const noexcept {
        for (const auto& variant : variants_) {
            if (variant.id == variant_id) {
                return &variant;
            }
        }
        return nullptr;
    }

private:
    ProductVariant* find_variant(const VariantId& variant_id) noexcept {
        for (auto& variant : variants_) {
            if (variant.id == variant_id) {
                return &variant;
            }
        }
        return nullptr;
    }

    ProductId id_;
    std::string name_;
    Category category_;
    std::string description_;
    std::string collection_;
    ProductStatus status_;
    std::vector<ProductVariant> variants_;
};

}  // namespace fashion_store::domain::catalog
