#pragma once

#include <algorithm>

#include "domain/notification/notification.hpp"

namespace fashion_store::application::notification {

using fashion_store::domain::notification::INotificationRepository;
using fashion_store::domain::notification::Notification;
using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::NotificationId;
using fashion_store::domain::shared::Result;

enum class NotificationServiceError {
    NotificationNotFound,
    NotificationAccessDenied
};

class NotificationApplicationService {
public:
    explicit NotificationApplicationService(INotificationRepository& notification_repository)
        : notification_repository_(notification_repository) {}

    std::vector<Notification> get_customer_notifications(const CustomerId& customer_id) const {
        auto notifications = notification_repository_.find_by_customer_id(customer_id);
        std::sort(notifications.begin(), notifications.end(), [](const Notification& left, const Notification& right) {
            return left.created_at_iso() > right.created_at_iso();
        });
        return notifications;
    }

    Result<Notification, NotificationServiceError> mark_read(const CustomerId& customer_id,
                                                             const NotificationId& notification_id) {
        auto notification = notification_repository_.find_by_id(notification_id);
        if (!notification.has_value()) {
            return Result<Notification, NotificationServiceError>::fail(NotificationServiceError::NotificationNotFound);
        }
        if (notification->customer_id() != customer_id) {
            return Result<Notification, NotificationServiceError>::fail(NotificationServiceError::NotificationAccessDenied);
        }
        notification->mark_read();
        notification_repository_.save(*notification);
        return Result<Notification, NotificationServiceError>::ok(*notification);
    }

private:
    INotificationRepository& notification_repository_;
};

}  // namespace fashion_store::application::notification
