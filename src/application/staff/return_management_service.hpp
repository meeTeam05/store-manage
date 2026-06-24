#pragma once

#include <chrono>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "domain/inventory/inventory_repository.hpp"
#include "domain/notification/notification.hpp"
#include "domain/order/order_repository.hpp"
#include "domain/returns/return_request.hpp"

namespace fashion_store::application::staff {

using fashion_store::domain::inventory::IInventoryRepository;
using fashion_store::domain::notification::INotificationRepository;
using fashion_store::domain::notification::Notification;
using fashion_store::domain::notification::NotificationType;
using fashion_store::domain::order::IOrderRepository;
using fashion_store::domain::returns::IReturnRepository;
using fashion_store::domain::returns::ReturnRequest;
using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::NotificationId;
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
                            IInventoryRepository& inventory_repository,
                            INotificationRepository& notification_repository)
        : return_repository_(return_repository),
          order_repository_(order_repository),
          inventory_repository_(inventory_repository),
          notification_repository_(notification_repository) {}

    Result<ReturnRequest, ReturnManagementError> approve_return(const ReturnId& return_id,
                                                                std::optional<std::string> note = std::nullopt) {
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
        notify_customer(updated_request, "Return approved", "Your return request has been approved by staff.", note);
        return Result<ReturnRequest, ReturnManagementError>::ok(updated_request);
    }

    Result<ReturnRequest, ReturnManagementError> reject_return(const ReturnId& return_id,
                                                               std::optional<std::string> note = std::nullopt) {
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
        notify_customer(updated_request, "Return rejected", "Your return request has been rejected by staff.", note);
        return Result<ReturnRequest, ReturnManagementError>::ok(updated_request);
    }

    Result<ReturnRequest, ReturnManagementError> restock_return(const ReturnId& return_id,
                                                                std::optional<std::string> note = std::nullopt) {
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

        return_repository_.save(updated_request);
        inventory_repository_.save(*inventory_item);
        notify_customer(updated_request, "Return restocked", "Returned items have been restocked into store inventory.", note);
        return Result<ReturnRequest, ReturnManagementError>::ok(updated_request);
    }

    Result<ReturnRequest, ReturnManagementError> refund_return(const ReturnId& return_id,
                                                               std::optional<std::string> refund_reference = std::nullopt) {
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
        notify_customer(updated_request, "Refund issued", "A refund has been issued for your return request.", refund_reference);
        return Result<ReturnRequest, ReturnManagementError>::ok(updated_request);
    }

    Result<ReturnRequest, ReturnManagementError> close_return(const ReturnId& return_id,
                                                              std::optional<std::string> note = std::nullopt) {
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
        notify_customer(updated_request, "Return case closed", "Your return case has been closed.", note);
        return Result<ReturnRequest, ReturnManagementError>::ok(updated_request);
    }

    std::vector<ReturnRequest> get_order_returns(const OrderId& order_id) const {
        return return_repository_.find_by_order_id(order_id);
    }

    std::vector<ReturnRequest> list_returns() const {
        std::vector<ReturnRequest> requests;
        for (const auto& order : order_repository_.list_all()) {
            const auto order_requests = return_repository_.find_by_order_id(order.id());
            requests.insert(requests.end(), order_requests.begin(), order_requests.end());
        }
        return requests;
    }

private:
    static std::string status_to_string(fashion_store::domain::returns::ReturnStatus status) {
        switch (status) {
            case fashion_store::domain::returns::ReturnStatus::Requested: return "Requested";
            case fashion_store::domain::returns::ReturnStatus::Approved: return "Approved";
            case fashion_store::domain::returns::ReturnStatus::Rejected: return "Rejected";
            case fashion_store::domain::returns::ReturnStatus::Restocked: return "Restocked";
            case fashion_store::domain::returns::ReturnStatus::Refunded: return "Refunded";
            case fashion_store::domain::returns::ReturnStatus::Closed: return "Closed";
        }
        return "Requested";
    }

    static std::string current_time_iso() {
        const auto now = std::chrono::system_clock::now();
        const auto tt = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &tt);
#else
        tm = *std::localtime(&tt);
#endif
        std::ostringstream output;
        output << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
        return output.str();
    }

    void notify_customer(const ReturnRequest& request,
                         const std::string& title,
                         const std::string& message,
                         const std::optional<std::string>& extra_detail) {
        auto order = order_repository_.find_by_id(request.order_id());
        if (!order.has_value()) {
            return;
        }
        const auto created_at = current_time_iso();
        Notification notification(
            NotificationId{request.id().value + "-" + status_to_string(request.status()) + "-" + created_at},
            CustomerId{order->customer_id().value},
            NotificationType::ReturnStatusUpdated,
            title,
            message,
            request.id(),
            status_to_string(request.status()),
            extra_detail,
            created_at,
            false);
        notification_repository_.save(notification);
    }

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
    INotificationRepository& notification_repository_;
};

}  // namespace fashion_store::application::staff
