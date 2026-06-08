#pragma once

#include <optional>

#include "domain/catalog/product_repository.hpp"

namespace fashion_store::application::catalog {

using fashion_store::domain::catalog::CatalogVariantView;
using fashion_store::domain::catalog::IProductRepository;
using fashion_store::domain::catalog::Product;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::VariantId;

class CatalogApplicationService {
public:
    explicit CatalogApplicationService(IProductRepository& product_repository)
        : product_repository_(product_repository) {}

    std::optional<Product> find_product(const ProductId& product_id) const {
        return product_repository_.find_by_id(product_id);
    }

    std::optional<CatalogVariantView> find_variant(const VariantId& variant_id) const {
        return product_repository_.find_variant(variant_id);
    }

private:
    IProductRepository& product_repository_;
};

}  // namespace fashion_store::application::catalog
