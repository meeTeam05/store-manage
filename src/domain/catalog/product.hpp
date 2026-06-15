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
            ProductStatus status = ProductStatus::Active);

    const ProductId& id() const noexcept { return id_; }
    const std::string& name() const noexcept { return name_; }
    const std::vector<ProductVariant>& variants() const noexcept { return variants_; }

    Status<ProductError> add_variant(const ProductVariantDraft& draft);

    const ProductVariant* find_variant(const VariantId& variant_id) const noexcept;

private:
    ProductVariant* find_variant(const VariantId& variant_id) noexcept;

    ProductId id_;
    std::string name_;
    Category category_;
    std::string description_;
    std::string collection_;
    ProductStatus status_;
    std::vector<ProductVariant> variants_;
};

}  // namespace fashion_store::domain::catalog
