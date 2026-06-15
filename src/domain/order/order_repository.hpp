#pragma once

#include <optional>
#include <vector>

#include "domain/order/order.hpp"

namespace fashion_store::domain::order {

class IOrderRepository {
public:
    virtual ~IOrderRepository() = default;

    virtual std::optional<Order> find_by_id(const OrderId& order_id) const = 0;
    virtual std::vector<Order> find_by_customer_id(const CustomerId& customer_id) const = 0;
    virtual std::vector<Order> list_all() const = 0;
    virtual void save(const Order& order) = 0;
};

}  // namespace fashion_store::domain::order
