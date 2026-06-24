#pragma once

#include <optional>
#include <string>
#include <vector>

#include "domain/shared/types.hpp"

namespace fashion_store::domain::notification {

using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::NotificationId;
using fashion_store::domain::shared::ReturnId;

enum class NotificationType {
    ReturnStatusUpdated
};

class Notification {
public:
    Notification(NotificationId id,
                 CustomerId customer_id,
                 NotificationType type,
                 std::string title,
                 std::string message,
                 std::optional<ReturnId> return_id,
                 std::string return_status,
                 std::optional<std::string> extra_detail,
                 std::string created_at_iso,
                 bool is_read = false)
        : id_(std::move(id)),
          customer_id_(std::move(customer_id)),
          type_(type),
          title_(std::move(title)),
          message_(std::move(message)),
          return_id_(std::move(return_id)),
          return_status_(std::move(return_status)),
          extra_detail_(std::move(extra_detail)),
          created_at_iso_(std::move(created_at_iso)),
          is_read_(is_read) {}

    const NotificationId& id() const noexcept { return id_; }
    const CustomerId& customer_id() const noexcept { return customer_id_; }
    NotificationType type() const noexcept { return type_; }
    const std::string& title() const noexcept { return title_; }
    const std::string& message() const noexcept { return message_; }
    const std::optional<ReturnId>& return_id() const noexcept { return return_id_; }
    const std::string& return_status() const noexcept { return return_status_; }
    const std::optional<std::string>& extra_detail() const noexcept { return extra_detail_; }
    const std::string& created_at_iso() const noexcept { return created_at_iso_; }
    bool is_read() const noexcept { return is_read_; }

    void mark_read() noexcept {
        is_read_ = true;
    }

private:
    NotificationId id_;
    CustomerId customer_id_;
    NotificationType type_;
    std::string title_;
    std::string message_;
    std::optional<ReturnId> return_id_;
    std::string return_status_;
    std::optional<std::string> extra_detail_;
    std::string created_at_iso_;
    bool is_read_{false};
};

class INotificationRepository {
public:
    virtual ~INotificationRepository() = default;

    virtual std::optional<Notification> find_by_id(const NotificationId& notification_id) const = 0;
    virtual std::vector<Notification> find_by_customer_id(const CustomerId& customer_id) const = 0;
    virtual void save(const Notification& notification) = 0;
};

}  // namespace fashion_store::domain::notification
