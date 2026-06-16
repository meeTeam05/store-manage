#pragma once

#include <optional>
#include <string>
#include <vector>

#include "application/cart/cart_application_service.hpp"
#include "application/catalog/catalog_application_service.hpp"
#include "application/customer/customer_application_service.hpp"
#include "application/identity/auth_application_service.hpp"
#include "application/order/order_application_service.hpp"

namespace fashion_store::api {

using fashion_store::application::cart::CartApplicationService;
using fashion_store::application::cart::CartServiceError;
using fashion_store::application::catalog::CatalogApplicationService;
using fashion_store::application::catalog::ProductSearchQuery;
using fashion_store::application::customer::CustomerApplicationService;
using fashion_store::application::identity::AuthApplicationService;
using fashion_store::application::identity::SignInError;
using fashion_store::application::order::OrderApplicationService;
using fashion_store::application::order::PlaceOrderCommand;
using fashion_store::application::order::PlaceOrderError;
using fashion_store::domain::catalog::Product;
using fashion_store::domain::shared::CartId;
using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::Result;
using fashion_store::domain::shared::VariantId;

enum class ApiError {
    AuthFailed,
    CartFailed,
    CheckoutFailed
};

struct ProductSummaryDto {
    std::string product_id;
    std::string name;
    std::string collection;
    int variant_count{0};
};

struct SignInDto {
    std::string account_id;
    std::string username;
    int role{0};
};

struct CartDto {
    std::string cart_id;
    int item_count{0};
    long long subtotal_minor{0};
};

struct CheckoutDto {
    std::string order_id;
    long long subtotal_minor{0};
    long long discount_minor{0};
    long long total_minor{0};
};

class FashionStoreApi {
public:
    FashionStoreApi(AuthApplicationService& auth_service,
                    CatalogApplicationService& catalog_service,
                    CartApplicationService& cart_service,
                    CustomerApplicationService& customer_service,
                    OrderApplicationService& order_service)
        : auth_service_(auth_service),
          catalog_service_(catalog_service),
          cart_service_(cart_service),
          customer_service_(customer_service),
          order_service_(order_service) {}

    Result<SignInDto, ApiError> sign_in(const std::string& username, const std::string& password_hash) const {
        auto result = auth_service_.sign_in(username, password_hash);
        if (!result) {
            return Result<SignInDto, ApiError>::fail(ApiError::AuthFailed);
        }
        return Result<SignInDto, ApiError>::ok(SignInDto{
            result.value().account.id().value,
            result.value().account.username(),
            static_cast<int>(result.value().role)
        });
    }

    std::vector<ProductSummaryDto> search_products(const ProductSearchQuery& query) const {
        std::vector<ProductSummaryDto> result;
        for (const auto& product : catalog_service_.search_products(query)) {
            result.push_back(to_summary(product));
        }
        return result;
    }

    Result<CartDto, ApiError> add_to_cart(const CartId& cart_id,
                                          const CustomerId& customer_id,
                                          const VariantId& variant_id,
                                          int quantity) {
        auto result = cart_service_.add_item(cart_id, customer_id, variant_id, quantity);
        if (!result) {
            return Result<CartDto, ApiError>::fail(ApiError::CartFailed);
        }
        return Result<CartDto, ApiError>::ok(CartDto{
            result.value().id().value,
            static_cast<int>(result.value().items().size()),
            result.value().subtotal().minor_units()
        });
    }

    Result<CheckoutDto, ApiError> checkout(const PlaceOrderCommand& command) {
        auto result = order_service_.place_order(command);
        if (!result) {
            return Result<CheckoutDto, ApiError>::fail(ApiError::CheckoutFailed);
        }
        return Result<CheckoutDto, ApiError>::ok(CheckoutDto{
            result.value().order.id().value,
            result.value().subtotal.minor_units(),
            result.value().discount.minor_units(),
            result.value().total.minor_units()
        });
    }

private:
    static ProductSummaryDto to_summary(const Product& product) {
        return ProductSummaryDto{
            product.id().value,
            product.name(),
            product.collection(),
            static_cast<int>(product.variants().size())
        };
    }

    AuthApplicationService& auth_service_;
    CatalogApplicationService& catalog_service_;
    CartApplicationService& cart_service_;
    CustomerApplicationService& customer_service_;
    OrderApplicationService& order_service_;
};

}  // namespace fashion_store::api
