#pragma once

#include <vector>

#include "domain/inventory/inventory_repository.hpp"
#include "domain/order/order_repository.hpp"
#include "domain/returns/return_request.hpp"

namespace fashion_store::application::staff {

using fashion_store::domain::inventory::IInventoryRepository;
using fashion_store::domain::order::IOrderRepository;
using fashion_store::domain::returns::IReturnRepository;
using fashion_store::domain::returns::ReturnRequest;
using fashion_store::domain::shared::OrderId;
using fashion_store::domain::shared::Result;
using fashion_store::domain::shared::ReturnId;

enum class ReturnManagementError {
    ReturnNotFound,
    OrderNotFound,
    OrderItemNotFound,
    InventoryNotFound,
    ReturnRuleViolation
};

class ReturnManagementService {
public:
    ReturnManagementService(IReturnRepository& return_repository,
                            IOrderRepository& order_repository,
                            IInventoryRepository& inventory_repository)
        : return_repository_(return_repository),
          order_repository_(order_repository),
          inventory_repository_(inventory_repository) {}

    Result<ReturnRequest, ReturnManagementError> approve_return(const ReturnId& return_id) {
        auto request = return_repository_.find_by_id(return_id);
        if (!request.has_value()) {
            return Result<ReturnRequest, ReturnManagementError>::fail(ReturnManagementError::ReturnNotFound);
        }

        auto updated_request = *request;
        auto approve_result = updated_request.approve();
        if (!approve_result) {
            return Result<ReturnRequest, ReturnManagementError>::fail(ReturnManagementError::ReturnRuleViolation);
        }

        return_repository_.save(updated_request);
        return Result<ReturnRequest, ReturnManagementError>::ok(updated_request);
    }

    Result<ReturnRequest, ReturnManagementError> reject_return(const ReturnId& return_id) {
        auto request = return_repository_.find_by_id(return_id);
        if (!request.has_value()) {
            return Result<ReturnRequest, ReturnManagementError>::fail(ReturnManagementError::ReturnNotFound);
        }

        auto updated_request = *request;
        auto reject_result = updated_request.reject();
        if (!reject_result) {
            return Result<ReturnRequest, ReturnManagementError>::fail(ReturnManagementError::ReturnRuleViolation);
        }

        return_repository_.save(updated_request);
        return Result<ReturnRequest, ReturnManagementError>::ok(updated_request);
    }

    Result<ReturnRequest, ReturnManagementError> restock_return(const ReturnId& return_id) {
        auto request = return_repository_.find_by_id(return_id);
        if (!request.has_value()) {
            return Result<ReturnRequest, ReturnManagementError>::fail(ReturnManagementError::ReturnNotFound);
        }

        auto order = order_repository_.find_by_id(request->order_id());
        if (!order.has_value()) {
            return Result<ReturnRequest, ReturnManagementError>::fail(ReturnManagementError::OrderNotFound);
        }

        const auto* matched_item = find_order_item(*order, *request);
        if (matched_item == nullptr) {
            return Result<ReturnRequest, ReturnManagementError>::fail(ReturnManagementError::OrderItemNotFound);
        }

        auto inventory_item = inventory_repository_.find_by_variant_id(matched_item->variant_id);
        if (!inventory_item.has_value()) {
            return Result<ReturnRequest, ReturnManagementError>::fail(ReturnManagementError::InventoryNotFound);
        }

        auto updated_request = *request;
        auto restock_state_result = updated_request.mark_restocked();
        if (!restock_state_result) {
            return Result<ReturnRequest, ReturnManagementError>::fail(ReturnManagementError::ReturnRuleViolation);
        }

        auto restock_inventory_result = inventory_item->restock(request->quantity());
        if (!restock_inventory_result) {
            return Result<ReturnRequest, ReturnManagementError>::fail(ReturnManagementError::ReturnRuleViolation);
        }

        inventory_repository_.save(*inventory_item);
        return_repository_.save(updated_request);
        return Result<ReturnRequest, ReturnManagementError>::ok(updated_request);
    }

    Result<ReturnRequest, ReturnManagementError> refund_return(const ReturnId& return_id) {
        auto request = return_repository_.find_by_id(return_id);
        if (!request.has_value()) {
            return Result<ReturnRequest, ReturnManagementError>::fail(ReturnManagementError::ReturnNotFound);
        }

        auto updated_request = *request;
        auto refund_result = updated_request.mark_refunded();
        if (!refund_result) {
            return Result<ReturnRequest, ReturnManagementError>::fail(ReturnManagementError::ReturnRuleViolation);
        }

        return_repository_.save(updated_request);
        return Result<ReturnRequest, ReturnManagementError>::ok(updated_request);
    }

    Result<ReturnRequest, ReturnManagementError> close_return(const ReturnId& return_id) {
        auto request = return_repository_.find_by_id(return_id);
        if (!request.has_value()) {
            return Result<ReturnRequest, ReturnManagementError>::fail(ReturnManagementError::ReturnNotFound);
        }

        auto updated_request = *request;
        auto close_result = updated_request.close();
        if (!close_result) {
            return Result<ReturnRequest, ReturnManagementError>::fail(ReturnManagementError::ReturnRuleViolation);
        }

        return_repository_.save(updated_request);
        return Result<ReturnRequest, ReturnManagementError>::ok(updated_request);
    }

    std::vector<ReturnRequest> get_order_returns(const OrderId& order_id) const {
        return return_repository_.find_by_order_id(order_id);
    }

private:
    static const fashion_store::domain::order::OrderItem* find_order_item(
        const fashion_store::domain::order::Order& order,
        const ReturnRequest& request) {
        for (const auto& item : order.items()) {
            if (item.id == request.order_item_id()) {
                return &item;
            }
        }
        return nullptr;
    }

    IReturnRepository& return_repository_;
    IOrderRepository& order_repository_;
    IInventoryRepository& inventory_repository_;
};

}  // namespace fashion_store::application::staff
