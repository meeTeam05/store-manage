#pragma once

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <vector>

#include "domain/catalog/product_repository.hpp"

namespace fashion_store::application::catalog {

using fashion_store::domain::catalog::CatalogVariantView;
using fashion_store::domain::catalog::Category;
using fashion_store::domain::catalog::IProductRepository;
using fashion_store::domain::catalog::Product;
using fashion_store::domain::catalog::ProductStatus;
using fashion_store::domain::shared::Color;
using fashion_store::domain::shared::Money;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::Size;
using fashion_store::domain::shared::VariantId;

enum class ProductSortMode {
    NameAsc,
    PriceAsc,
    PriceDesc
};

struct ProductSearchQuery {
    std::optional<std::string> keyword;
    std::optional<Category> category;
    std::optional<Size> size;
    std::optional<Color> color;
    std::optional<Money> min_price;
    std::optional<Money> max_price;
    std::optional<std::string> collection;
    std::optional<ProductStatus> status;
    ProductSortMode sort_mode{ProductSortMode::NameAsc};
};

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

    std::vector<Product> search_products(const ProductSearchQuery& query) const {
        auto products = product_repository_.list_all();
        std::vector<Product> result;
        for (const auto& product : products) {
            if (matches(product, query)) {
                result.push_back(product);
            }
        }

        std::sort(result.begin(), result.end(), [&](const Product& lhs, const Product& rhs) {
            switch (query.sort_mode) {
                case ProductSortMode::PriceAsc:
                    return lowest_variant_price(lhs) < lowest_variant_price(rhs);
                case ProductSortMode::PriceDesc:
                    return lowest_variant_price(lhs) > lowest_variant_price(rhs);
                case ProductSortMode::NameAsc:
                default:
                    return lhs.name() < rhs.name();
            }
        });

        return result;
    }

private:
    static bool matches(const Product& product, const ProductSearchQuery& query) {
        if (query.keyword.has_value() && !contains_case_insensitive(product.name(), *query.keyword) &&
            !contains_case_insensitive(product.description(), *query.keyword)) {
            return false;
        }
        if (query.category.has_value() && product.category() != *query.category) {
            return false;
        }
        if (query.collection.has_value() && product.collection() != *query.collection) {
            return false;
        }
        if (query.status.has_value() && product.status() != *query.status) {
            return false;
        }

        if (query.size.has_value() || query.color.has_value() ||
            query.min_price.has_value() || query.max_price.has_value()) {
            return has_matching_variant(product, query);
        }

        return true;
    }

    static bool has_matching_variant(const Product& product, const ProductSearchQuery& query) {
        for (const auto& variant : product.variants()) {
            if (!variant.active) {
                continue;
            }
            if (query.size.has_value() && variant.size != *query.size) {
                continue;
            }
            if (query.color.has_value() && variant.color != *query.color) {
                continue;
            }
            if (query.min_price.has_value() && variant.price < *query.min_price) {
                continue;
            }
            if (query.max_price.has_value() && variant.price > *query.max_price) {
                continue;
            }
            return true;
        }
        return false;
    }

    static Money lowest_variant_price(const Product& product) {
        auto lowest = Money::from_minor(0);
        bool has_price = false;
        for (const auto& variant : product.variants()) {
            if (!variant.active) {
                continue;
            }
            if (!has_price || variant.price < lowest) {
                lowest = variant.price;
                has_price = true;
            }
        }
        return lowest;
    }

    static bool contains_case_insensitive(const std::string& haystack, const std::string& needle) {
        return lowercase(haystack).find(lowercase(needle)) != std::string::npos;
    }

    static std::string lowercase(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    }

    IProductRepository& product_repository_;
};

}  // namespace fashion_store::application::catalog
