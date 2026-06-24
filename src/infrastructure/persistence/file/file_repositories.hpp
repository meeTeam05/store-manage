#pragma once

#include <filesystem>

#include "domain/cart/cart_repository.hpp"
#include "domain/catalog/product_repository.hpp"
#include "domain/customer/customer.hpp"
#include "domain/identity/account.hpp"
#include "domain/inventory/inventory_repository.hpp"
#include "domain/notification/notification.hpp"
#include "domain/order/order_repository.hpp"
#include "domain/pricing/voucher_repository.hpp"
#include "domain/review/review.hpp"
#include "domain/returns/return_request.hpp"
#include "domain/staff/employee.hpp"

namespace fashion_store::infrastructure::persistence::file {

class FileProductRepository : public fashion_store::domain::catalog::IProductRepository {
public:
    explicit FileProductRepository(std::filesystem::path path);

    std::optional<fashion_store::domain::catalog::Product> find_by_id(
        const fashion_store::domain::shared::ProductId& product_id) const override;
    std::optional<fashion_store::domain::catalog::CatalogVariantView> find_variant(
        const fashion_store::domain::shared::VariantId& variant_id) const override;
    std::vector<fashion_store::domain::catalog::Product> list_all() const override;
    void save(const fashion_store::domain::catalog::Product& product) override;

private:
    std::filesystem::path path_;
};

class FileInventoryRepository : public fashion_store::domain::inventory::IInventoryRepository {
public:
    explicit FileInventoryRepository(std::filesystem::path path);

    std::optional<fashion_store::domain::inventory::InventoryItem> find_by_variant_id(
        const fashion_store::domain::shared::VariantId& variant_id) const override;
    std::vector<fashion_store::domain::inventory::InventoryItem> list_all() const override;
    void save(const fashion_store::domain::inventory::InventoryItem& item) override;

private:
    std::filesystem::path path_;
};

class FileCartRepository : public fashion_store::domain::cart::ICartRepository {
public:
    explicit FileCartRepository(std::filesystem::path path);

    std::optional<fashion_store::domain::cart::Cart> find_by_id(
        const fashion_store::domain::shared::CartId& cart_id) const override;
    std::optional<fashion_store::domain::cart::Cart> find_by_customer_id(
        const fashion_store::domain::shared::CustomerId& customer_id) const override;
    void save(const fashion_store::domain::cart::Cart& cart) override;

private:
    std::filesystem::path path_;
};

class FileOrderRepository : public fashion_store::domain::order::IOrderRepository {
public:
    explicit FileOrderRepository(std::filesystem::path path);

    std::optional<fashion_store::domain::order::Order> find_by_id(
        const fashion_store::domain::shared::OrderId& order_id) const override;
    std::vector<fashion_store::domain::order::Order> find_by_customer_id(
        const fashion_store::domain::shared::CustomerId& customer_id) const override;
    std::vector<fashion_store::domain::order::Order> list_all() const override;
    void save(const fashion_store::domain::order::Order& order) override;

private:
    std::filesystem::path path_;
};

class FileVoucherRepository : public fashion_store::domain::pricing::IVoucherRepository {
public:
    explicit FileVoucherRepository(std::filesystem::path path);

    std::optional<fashion_store::domain::pricing::Voucher> find_by_code(std::string_view code) const override;
    void save(const fashion_store::domain::pricing::Voucher& voucher) override;

private:
    std::filesystem::path path_;
};

class FileAccountRepository : public fashion_store::domain::identity::IAccountRepository {
public:
    explicit FileAccountRepository(std::filesystem::path path);

    std::optional<fashion_store::domain::identity::Account> find_by_id(
        const fashion_store::domain::shared::AccountId& account_id) const override;
    std::optional<fashion_store::domain::identity::Account> find_by_username(
        const std::string& username) const override;
    std::vector<fashion_store::domain::identity::Account> list_all() const override;
    void save(const fashion_store::domain::identity::Account& account) override;

private:
    std::filesystem::path path_;
};

class FileCustomerRepository : public fashion_store::domain::customer::ICustomerRepository {
public:
    explicit FileCustomerRepository(std::filesystem::path path);

    std::optional<fashion_store::domain::customer::Customer> find_by_id(
        const fashion_store::domain::shared::CustomerId& customer_id) const override;
    std::optional<fashion_store::domain::customer::Customer> find_by_account_id(
        const fashion_store::domain::shared::AccountId& account_id) const override;
    void save(const fashion_store::domain::customer::Customer& customer) override;

private:
    std::filesystem::path path_;
};

class FileEmployeeRepository : public fashion_store::domain::staff::IEmployeeRepository {
public:
    explicit FileEmployeeRepository(std::filesystem::path path);

    std::optional<fashion_store::domain::staff::Employee> find_by_id(
        const fashion_store::domain::shared::EmployeeId& employee_id) const override;
    std::optional<fashion_store::domain::staff::Employee> find_by_account_id(
        const fashion_store::domain::shared::AccountId& account_id) const override;
    std::vector<fashion_store::domain::staff::Employee> list_all() const override;
    void save(const fashion_store::domain::staff::Employee& employee) override;

private:
    std::filesystem::path path_;
};

class FileReviewRepository : public fashion_store::domain::review::IReviewRepository {
public:
    explicit FileReviewRepository(std::filesystem::path path);

    std::vector<fashion_store::domain::review::Review> find_by_product_id(
        const fashion_store::domain::shared::ProductId& product_id) const override;
    std::vector<fashion_store::domain::review::Review> find_by_customer_id(
        const fashion_store::domain::shared::CustomerId& customer_id) const override;
    void save(const fashion_store::domain::review::Review& review) override;

private:
    std::filesystem::path path_;
};

class FileReturnRepository : public fashion_store::domain::returns::IReturnRepository {
public:
    explicit FileReturnRepository(std::filesystem::path path);

    std::optional<fashion_store::domain::returns::ReturnRequest> find_by_id(
        const fashion_store::domain::shared::ReturnId& return_id) const override;
    std::vector<fashion_store::domain::returns::ReturnRequest> find_by_order_id(
        const fashion_store::domain::shared::OrderId& order_id) const override;
    void save(const fashion_store::domain::returns::ReturnRequest& request) override;

private:
    std::filesystem::path path_;
};

class FileNotificationRepository : public fashion_store::domain::notification::INotificationRepository {
public:
    explicit FileNotificationRepository(std::filesystem::path path);

    std::optional<fashion_store::domain::notification::Notification> find_by_id(
        const fashion_store::domain::shared::NotificationId& notification_id) const override;
    std::vector<fashion_store::domain::notification::Notification> find_by_customer_id(
        const fashion_store::domain::shared::CustomerId& customer_id) const override;
    void save(const fashion_store::domain::notification::Notification& notification) override;

private:
    std::filesystem::path path_;
};

}  // namespace fashion_store::infrastructure::persistence::file
