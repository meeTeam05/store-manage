#pragma once

#include <string>
#include <vector>

#include "domain/order/order_repository.hpp"
#include "domain/returns/return_policy.hpp"
#include "domain/returns/return_request.hpp"

namespace fashion_store::application::returns {

using fashion_store::domain::order::IOrderRepository;
using fashion_store::domain::returns::IReturnRepository;
using fashion_store::domain::returns::ReturnEligibilityError;
using fashion_store::domain::returns::ReturnPolicy;
using fashion_store::domain::returns::ReturnRequest;
using fashion_store::domain::shared::OrderId;
using fashion_store::domain::shared::OrderItemId;
using fashion_store::domain::shared::Result;
using fashion_store::domain::shared::ReturnId;

enum class CreateReturnError {
    OrderNotFound,
    QuantityExceededByExistingRequests,
    ReturnNotEligible
};

class ReturnsApplicationService {
public:
    ReturnsApplicationService(IOrderRepository& order_repository, IReturnRepository& return_repository)
        : order_repository_(order_repository), return_repository_(return_repository) {}

    Result<ReturnRequest, CreateReturnError> create_return_request(const OrderId& order_id,
                                                                   ReturnId return_id,
                                                                   const OrderItemId& order_item_id,
                                                                   int quantity,
                                                                   std::string reason) {
        auto order = order_repository_.find_by_id(order_id);
        if (!order.has_value()) {
            return Result<ReturnRequest, CreateReturnError>::fail(CreateReturnError::OrderNotFound);
        }

        int purchased_quantity = 0;
        for (const auto& item : order->items()) {
            if (item.id == order_item_id) {
                purchased_quantity = item.quantity;
                break;
            }
        }

        int existing_requested_quantity = 0;
        for (const auto& existing_request : return_repository_.find_by_order_id(order_id)) {
            if (existing_request.order_item_id() == order_item_id) {
                existing_requested_quantity += existing_request.quantity();
            }
        }

        if (purchased_quantity > 0 &&
            existing_requested_quantity + quantity > purchased_quantity) {
            return Result<ReturnRequest, CreateReturnError>::fail(
                CreateReturnError::QuantityExceededByExistingRequests);
        }

        auto request = ReturnPolicy::create_request_for_order_item(
            std::move(return_id),
            *order,
            order_item_id,
            quantity,
            std::move(reason));
        if (!request) {
            return Result<ReturnRequest, CreateReturnError>::fail(CreateReturnError::ReturnNotEligible);
        }

        return_repository_.save(request.value());
        return Result<ReturnRequest, CreateReturnError>::ok(request.value());
    }

    std::vector<ReturnRequest> get_order_returns(const OrderId& order_id) const {
        return return_repository_.find_by_order_id(order_id);
    }

private:
    IOrderRepository& order_repository_;
    IReturnRepository& return_repository_;
};

}  // namespace fashion_store::application::returns
