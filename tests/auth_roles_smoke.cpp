#include <cassert>

#include "application/identity/auth_application_service.hpp"
#include "domain/identity/access_control.hpp"
#include "domain/identity/account.hpp"
#include "domain/staff/employee.hpp"
#include "infrastructure/persistence/in_memory/in_memory_repositories.hpp"

int main() {
    using namespace fashion_store::application::identity;
    using namespace fashion_store::domain::identity;
    using namespace fashion_store::domain::staff;
    using namespace fashion_store::domain::shared;
    using namespace fashion_store::infrastructure::persistence::in_memory;

    InMemoryAccountRepository account_repository;
    InMemoryEmployeeRepository employee_repository;
    account_repository.save(Account(AccountId{"account-customer"}, "client001", "hash:123456", Role::Customer, AccountStatus::Active));
    account_repository.save(Account(AccountId{"account-staff"}, "staff001", "hash:staff123", Role::Staff, AccountStatus::Active));
    account_repository.save(Account(AccountId{"account-manager"}, "manager001", "hash:manager123", Role::Manager, AccountStatus::Active));
    account_repository.save(Account(AccountId{"account-admin"}, "admin001", "hash:admin123", Role::Admin, AccountStatus::Active));
    account_repository.save(Account(AccountId{"account-locked"}, "locked001", "hash:locked123", Role::Customer, AccountStatus::Locked));
    employee_repository.save(Employee(EmployeeId{"employee-staff"}, AccountId{"account-staff"}, "Store Staff", "Sales Staff"));
    employee_repository.save(Employee(EmployeeId{"employee-manager"}, AccountId{"account-manager"}, "Store Manager", "Manager"));
    employee_repository.save(Employee(EmployeeId{"employee-admin"}, AccountId{"account-admin"}, "System Admin", "Administrator"));

    AuthApplicationService auth_service(account_repository);

    auto customer = auth_service.sign_in("client001", "hash:123456");
    assert(customer);
    assert(customer.value().role == Role::Customer);

    auto staff = auth_service.sign_in("staff001", "hash:staff123");
    assert(staff);
    assert(staff.value().role == Role::Staff);

    auto admin = auth_service.sign_in("admin001", "hash:admin123");
    assert(admin);
    assert(admin.value().role == Role::Admin);

    auto manager = auth_service.sign_in("manager001", "hash:manager123");
    assert(manager);
    assert(manager.value().role == Role::Manager);

    auto locked = auth_service.sign_in("locked001", "hash:locked123");
    assert(!locked);
    assert(locked.error() == SignInError::AccountLocked);

    auto wrong_password = auth_service.sign_in("admin001", "wrong");
    assert(!wrong_password);
    assert(wrong_password.error() == SignInError::InvalidCredentials);

    assert(AccessControlPolicy::can_access_customer_portal(Role::Customer));
    assert(!AccessControlPolicy::can_access_customer_portal(Role::Staff));
    assert(AccessControlPolicy::can_access_staff_workspace(Role::Staff));
    assert(!AccessControlPolicy::can_access_staff_workspace(Role::Admin));
    assert(!AccessControlPolicy::can_access_staff_workspace(Role::Manager));
    assert(!AccessControlPolicy::can_access_staff_workspace(Role::Customer));
    assert(AccessControlPolicy::can_access_admin_console(Role::Admin));
    assert(AccessControlPolicy::can_access_admin_console(Role::Manager));
    assert(!AccessControlPolicy::can_access_admin_console(Role::Staff));
    assert(!AccessControlPolicy::can_manage_catalog(Role::Staff));
    assert(AccessControlPolicy::can_manage_catalog(Role::Manager));
    assert(AccessControlPolicy::can_manage_catalog(Role::Admin));
    assert(!AccessControlPolicy::can_manage_inventory(Role::Staff));
    assert(AccessControlPolicy::can_manage_inventory(Role::Manager));
    assert(AccessControlPolicy::can_manage_inventory(Role::Admin));
    assert(AccessControlPolicy::can_manage_orders(Role::Staff));
    assert(!AccessControlPolicy::can_manage_orders(Role::Admin));
    assert(AccessControlPolicy::can_manage_returns(Role::Staff));
    assert(!AccessControlPolicy::can_manage_returns(Role::Admin));
    assert(!AccessControlPolicy::can_manage_vouchers(Role::Staff));
    assert(AccessControlPolicy::can_manage_vouchers(Role::Manager));
    assert(AccessControlPolicy::can_manage_vouchers(Role::Admin));
    assert(AccessControlPolicy::can_view_reports(Role::Manager));
    assert(AccessControlPolicy::can_view_reports(Role::Admin));
    assert(AccessControlPolicy::can_manage_accounts(Role::Manager));
    assert(AccessControlPolicy::can_manage_accounts(Role::Admin));
    assert(!AccessControlPolicy::can_manage_accounts(Role::Staff));

    auto staff_employee = employee_repository.find_by_account_id(AccountId{"account-staff"});
    assert(staff_employee);
    assert(staff_employee->id() == EmployeeId{"employee-staff"});
    assert(staff_employee->position() == "Sales Staff");

    auto manager_employee = employee_repository.find_by_account_id(AccountId{"account-manager"});
    assert(manager_employee);
    assert(manager_employee->id() == EmployeeId{"employee-manager"});
    assert(manager_employee->position() == "Manager");

    return 0;
}
