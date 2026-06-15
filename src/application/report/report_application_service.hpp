#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "domain/inventory/inventory_repository.hpp"
#include "domain/order/order_repository.hpp"

namespace fashion_store::application::report {

using fashion_store::domain::inventory::IInventoryRepository;
using fashion_store::domain::order::IOrderRepository;
using fashion_store::domain::order::OrderStatus;
using fashion_store::domain::order::PaymentStatus;
using fashion_store::domain::shared::Money;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::VariantId;

struct RevenueReport {
    int order_count{0};
    int item_count{0};
    Money total_revenue{Money::from_minor(0)};
};

struct ProductSalesReport {
    ProductId product_id;
    std::string product_name;
    int quantity_sold{0};
    Money gross_sales{Money::from_minor(0)};
};

struct LowStockReport {
    VariantId variant_id;
    int on_hand{0};
    int reserved{0};
    int available{0};
};

class ReportApplicationService {
public:
    ReportApplicationService(IOrderRepository& order_repository,
                             IInventoryRepository& inventory_repository)
        : order_repository_(order_repository),
          inventory_repository_(inventory_repository) {}

    RevenueReport build_revenue_report() const {
        RevenueReport report;
        for (const auto& order : order_repository_.list_all()) {
            if (!is_revenue_order(order.status(), order.payment_status())) {
                continue;
            }

            ++report.order_count;
            report.total_revenue = report.total_revenue + order.total();
            for (const auto& item : order.items()) {
                report.item_count += item.quantity;
            }
        }
        return report;
    }

    std::vector<ProductSalesReport> build_best_selling_products() const {
        struct Accumulator {
            ProductId product_id;
            std::string product_name;
            int quantity_sold{0};
            Money gross_sales{Money::from_minor(0)};
        };

        std::unordered_map<std::string, Accumulator> by_product;
        for (const auto& order : order_repository_.list_all()) {
            if (!is_revenue_order(order.status(), order.payment_status())) {
                continue;
            }

            for (const auto& item : order.items()) {
                auto& aggregate = by_product.try_emplace(
                    item.product_id.value,
                    Accumulator{item.product_id, item.product_name, 0, Money::from_minor(0)}).first->second;
                aggregate.quantity_sold += item.quantity;
                aggregate.gross_sales = aggregate.gross_sales + item.line_total();
            }
        }

        std::vector<ProductSalesReport> result;
        result.reserve(by_product.size());
        for (const auto& [_, aggregate] : by_product) {
            result.push_back(ProductSalesReport{
                aggregate.product_id,
                aggregate.product_name,
                aggregate.quantity_sold,
                aggregate.gross_sales
            });
        }

        std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
            if (lhs.quantity_sold != rhs.quantity_sold) {
                return lhs.quantity_sold > rhs.quantity_sold;
            }
            return lhs.gross_sales > rhs.gross_sales;
        });
        return result;
    }

    std::vector<LowStockReport> build_low_stock_report(int threshold) const {
        std::vector<LowStockReport> result;
        for (const auto& item : inventory_repository_.list_all()) {
            if (item.available() <= threshold) {
                result.push_back(LowStockReport{
                    item.variant_id(),
                    item.on_hand(),
                    item.reserved(),
                    item.available()
                });
            }
        }

        std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
            if (lhs.available != rhs.available) {
                return lhs.available < rhs.available;
            }
            return lhs.variant_id.value < rhs.variant_id.value;
        });
        return result;
    }

private:
    static bool is_revenue_order(OrderStatus status, PaymentStatus payment_status) noexcept {
        return payment_status == PaymentStatus::Paid &&
               status != OrderStatus::Cancelled;
    }

    IOrderRepository& order_repository_;
    IInventoryRepository& inventory_repository_;
};

}  // namespace fashion_store::application::report
