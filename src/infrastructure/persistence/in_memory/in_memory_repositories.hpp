#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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

namespace fashion_store::infrastructure::persistence::in_memory {

using fashion_store::domain::cart::Cart;
using fashion_store::domain::cart::ICartRepository;
using fashion_store::domain::catalog::CatalogVariantView;
using fashion_store::domain::catalog::IProductRepository;
using fashion_store::domain::catalog::Product;
using fashion_store::domain::customer::Customer;
using fashion_store::domain::customer::ICustomerRepository;
using fashion_store::domain::identity::Account;
using fashion_store::domain::identity::IAccountRepository;
using fashion_store::domain::inventory::IInventoryRepository;
using fashion_store::domain::inventory::InventoryItem;
using fashion_store::domain::notification::INotificationRepository;
using fashion_store::domain::notification::Notification;
using fashion_store::domain::order::IOrderRepository;
using fashion_store::domain::order::Order;
using fashion_store::domain::pricing::IVoucherRepository;
using fashion_store::domain::pricing::Voucher;
using fashion_store::domain::review::IReviewRepository;
using fashion_store::domain::review::Review;
using fashion_store::domain::returns::IReturnRepository;
using fashion_store::domain::returns::ReturnRequest;
using fashion_store::domain::staff::Employee;
using fashion_store::domain::staff::IEmployeeRepository;
using fashion_store::domain::shared::CartId;
using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::AccountId;
using fashion_store::domain::shared::EmployeeId;
using fashion_store::domain::shared::NotificationId;
using fashion_store::domain::shared::OrderId;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::ReturnId;
using fashion_store::domain::shared::VariantId;

class InMemoryProductRepository : public IProductRepository {
public:
    std::optional<Product> find_by_id(const ProductId& product_id) const override {
        auto it = products_.find(product_id.value);
        if (it == products_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<CatalogVariantView> find_variant(const VariantId& variant_id) const override {
        for (const auto& [_, product] : products_) {
            const auto* variant = product.find_variant(variant_id);
            if (variant != nullptr) {
                return CatalogVariantView{product.id(), product.name(), product.status(), *variant};
            }
        }
        return std::nullopt;
    }

    std::vector<Product> list_all() const override {
        std::vector<Product> products;
        products.reserve(products_.size());
        for (const auto& [_, product] : products_) {
            products.push_back(product);
        }
        return products;
    }

    void save(const Product& product) override {
        products_.insert_or_assign(product.id().value, product);
    }

private:
    std::unordered_map<std::string, Product> products_;
};

class InMemoryCustomerRepository : public ICustomerRepository {
public:
    std::optional<Customer> find_by_id(const CustomerId& customer_id) const override {
        auto it = customers_.find(customer_id.value);
        if (it == customers_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<Customer> find_by_account_id(const AccountId& account_id) const override {
        for (const auto& [_, customer] : customers_) {
            if (customer.account_id() == account_id) {
                return customer;
            }
        }
        return std::nullopt;
    }

    void save(const Customer& customer) override {
        customers_.insert_or_assign(customer.id().value, customer);
    }

private:
    std::unordered_map<std::string, Customer> customers_;
};

class InMemoryAccountRepository : public IAccountRepository {
public:
    std::optional<Account> find_by_id(const AccountId& account_id) const override {
        auto it = accounts_.find(account_id.value);
        if (it == accounts_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<Account> find_by_username(const std::string& username) const override {
        for (const auto& [_, account] : accounts_) {
            if (account.username() == username) {
                return account;
            }
        }
        return std::nullopt;
    }

    std::vector<Account> list_all() const override {
        std::vector<Account> accounts;
        accounts.reserve(accounts_.size());
        for (const auto& [_, account] : accounts_) {
            accounts.push_back(account);
        }
        return accounts;
    }

    void save(const Account& account) override {
        accounts_.insert_or_assign(account.id().value, account);
    }

private:
    std::unordered_map<std::string, Account> accounts_;
};

class InMemoryEmployeeRepository : public IEmployeeRepository {
public:
    std::optional<Employee> find_by_id(const EmployeeId& employee_id) const override {
        auto it = employees_.find(employee_id.value);
        if (it == employees_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<Employee> find_by_account_id(const AccountId& account_id) const override {
        for (const auto& [_, employee] : employees_) {
            if (employee.account_id() == account_id) {
                return employee;
            }
        }
        return std::nullopt;
    }

    std::vector<Employee> list_all() const override {
        std::vector<Employee> employees;
        employees.reserve(employees_.size());
        for (const auto& [_, employee] : employees_) {
            employees.push_back(employee);
        }
        return employees;
    }

    void save(const Employee& employee) override {
        employees_.insert_or_assign(employee.id().value, employee);
    }

private:
    std::unordered_map<std::string, Employee> employees_;
};

class InMemoryInventoryRepository : public IInventoryRepository {
public:
    std::optional<InventoryItem> find_by_variant_id(const VariantId& variant_id) const override {
        auto it = items_.find(variant_id.value);
        if (it == items_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::vector<InventoryItem> list_all() const override {
        std::vector<InventoryItem> items;
        items.reserve(items_.size());
        for (const auto& [_, item] : items_) {
            items.push_back(item);
        }
        return items;
    }

    void save(const InventoryItem& item) override {
        items_.insert_or_assign(item.variant_id().value, item);
    }

private:
    std::unordered_map<std::string, InventoryItem> items_;
};

class InMemoryCartRepository : public ICartRepository {
public:
    std::optional<Cart> find_by_id(const CartId& cart_id) const override {
        auto it = carts_.find(cart_id.value);
        if (it == carts_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<Cart> find_by_customer_id(const CustomerId& customer_id) const override {
        for (const auto& [_, cart] : carts_) {
            if (cart.customer_id() == customer_id) {
                return cart;
            }
        }
        return std::nullopt;
    }

    void save(const Cart& cart) override {
        carts_.insert_or_assign(cart.id().value, cart);
    }

private:
    std::unordered_map<std::string, Cart> carts_;
};

class InMemoryOrderRepository : public IOrderRepository {
public:
    std::optional<Order> find_by_id(const OrderId& order_id) const override {
        auto it = orders_.find(order_id.value);
        if (it == orders_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::vector<Order> find_by_customer_id(const CustomerId& customer_id) const override {
        std::vector<Order> orders;
        for (const auto& [_, order] : orders_) {
            if (order.customer_id() == customer_id) {
                orders.push_back(order);
            }
        }
        return orders;
    }

    std::vector<Order> list_all() const override {
        std::vector<Order> orders;
        orders.reserve(orders_.size());
        for (const auto& [_, order] : orders_) {
            orders.push_back(order);
        }
        return orders;
    }

    void save(const Order& order) override {
        orders_.insert_or_assign(order.id().value, order);
    }

private:
    std::unordered_map<std::string, Order> orders_;
};

class InMemoryVoucherRepository : public IVoucherRepository {
public:
    std::optional<Voucher> find_by_code(std::string_view code) const override {
        auto it = vouchers_.find(std::string(code));
        if (it == vouchers_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void save(const Voucher& voucher) override {
        vouchers_.insert_or_assign(voucher.code(), voucher);
    }

private:
    std::unordered_map<std::string, Voucher> vouchers_;
};

class InMemoryReviewRepository : public IReviewRepository {
public:
    std::vector<Review> find_by_product_id(const ProductId& product_id) const override {
        std::vector<Review> result;
        for (const auto& review : reviews_) {
            if (review.product_id() == product_id) {
                result.push_back(review);
            }
        }
        return result;
    }

    std::vector<Review> find_by_customer_id(const CustomerId& customer_id) const override {
        std::vector<Review> result;
        for (const auto& review : reviews_) {
            if (review.customer_id() == customer_id) {
                result.push_back(review);
            }
        }
        return result;
    }

    void save(const Review& review) override {
        reviews_.push_back(review);
    }

private:
    std::vector<Review> reviews_;
};

class InMemoryReturnRepository : public IReturnRepository {
public:
    std::optional<ReturnRequest> find_by_id(const ReturnId& return_id) const override {
        for (const auto& request : requests_) {
            if (request.id() == return_id) {
                return request;
            }
        }
        return std::nullopt;
    }

    std::vector<ReturnRequest> find_by_order_id(const OrderId& order_id) const override {
        std::vector<ReturnRequest> result;
        for (const auto& request : requests_) {
            if (request.order_id() == order_id) {
                result.push_back(request);
            }
        }
        return result;
    }

    void save(const ReturnRequest& request) override {
        for (auto& existing : requests_) {
            if (existing.id() == request.id()) {
                existing = request;
                return;
            }
        }
        requests_.push_back(request);
    }

private:
    std::vector<ReturnRequest> requests_;
};

class InMemoryNotificationRepository : public INotificationRepository {
public:
    std::optional<Notification> find_by_id(const NotificationId& notification_id) const override {
        for (const auto& notification : notifications_) {
            if (notification.id() == notification_id) {
                return notification;
            }
        }
        return std::nullopt;
    }

    std::vector<Notification> find_by_customer_id(const CustomerId& customer_id) const override {
        std::vector<Notification> result;
        for (const auto& notification : notifications_) {
            if (notification.customer_id() == customer_id) {
                result.push_back(notification);
            }
        }
        return result;
    }

    void save(const Notification& notification) override {
        for (auto& existing : notifications_) {
            if (existing.id() == notification.id()) {
                existing = notification;
                return;
            }
        }
        notifications_.push_back(notification);
    }

private:
    std::vector<Notification> notifications_;
};

}  // namespace fashion_store::infrastructure::persistence::in_memory
