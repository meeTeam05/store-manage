#pragma once

#include "domain/identity/account.hpp"

namespace fashion_store::domain::identity {

class AccessControlPolicy {
public:
    static bool can_access_customer_portal(Role role) noexcept {
        return role == Role::Customer;
    }

    static bool can_access_staff_workspace(Role role) noexcept {
        return role == Role::Staff;
    }

    static bool can_access_admin_console(Role role) noexcept {
        return role == Role::Admin ||
               role == Role::Manager;
    }

    static bool can_manage_catalog(Role role) noexcept {
        return role == Role::Manager ||
               role == Role::Admin;
    }

    static bool can_manage_inventory(Role role) noexcept {
        return role == Role::Admin ||
               role == Role::Manager;
    }

    static bool can_manage_orders(Role role) noexcept {
        return role == Role::Staff;
    }

    static bool can_manage_returns(Role role) noexcept {
        return role == Role::Staff;
    }

    static bool can_manage_vouchers(Role role) noexcept {
        return role == Role::Manager ||
               role == Role::Admin;
    }

    static bool can_view_reports(Role role) noexcept {
        return role == Role::Manager ||
               role == Role::Admin;
    }

    static bool can_manage_accounts(Role role) noexcept {
        return role == Role::Manager ||
               role == Role::Admin;
    }
};

}  // namespace fashion_store::domain::identity
