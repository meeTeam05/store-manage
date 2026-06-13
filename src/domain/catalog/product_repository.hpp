#pragma once

#include <optional>

#include "domain/catalog/product.hpp"

namespace fashion_store::domain::catalog {

struct CatalogVariantView {
    ProductId product_id;
    std::string product_name;
    ProductVariant variant;
};

class IProductRepository {
public:
    virtual ~IProductRepository() = default;

    virtual std::optional<Product> find_by_id(const ProductId& product_id) const = 0;
    virtual std::optional<CatalogVariantView> find_variant(const VariantId& variant_id) const = 0;
    virtual void save(const Product& product) = 0;
};

}  // namespace fashion_store::domain::catalog
