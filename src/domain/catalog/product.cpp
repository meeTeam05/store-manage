#include "domain/catalog/product.hpp"

namespace fashion_store::domain::catalog {

Product::Product(ProductId id,
                 std::string name,
                 Category category,
                 std::string description,
                 std::string collection,
                 ProductStatus status)
    : id_(std::move(id)),
      name_(std::move(name)),
      category_(category),
      description_(std::move(description)),
      collection_(std::move(collection)),
      status_(status) {
    require(!id_.value.empty(), "product id must not be empty");
    require(!name_.empty(), "product name must not be empty");
}

Product Product::rehydrate(ProductId id,
                           std::string name,
                           Category category,
                           std::string description,
                           std::string collection,
                           ProductStatus status,
                           std::vector<ProductVariant> variants) {
    Product product(
        std::move(id),
        std::move(name),
        category,
        std::move(description),
        std::move(collection),
        status);
    product.variants_ = std::move(variants);
    return product;
}

Status<ProductError> Product::add_variant(const ProductVariantDraft& draft) {
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

const ProductVariant* Product::find_variant(const VariantId& variant_id) const noexcept {
    for (const auto& variant : variants_) {
        if (variant.id == variant_id) {
            return &variant;
        }
    }
    return nullptr;
}

ProductVariant* Product::find_variant(const VariantId& variant_id) noexcept {
    for (auto& variant : variants_) {
        if (variant.id == variant_id) {
            return &variant;
        }
    }
    return nullptr;
}

}  // namespace fashion_store::domain::catalog
