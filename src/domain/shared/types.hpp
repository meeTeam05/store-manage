#pragma once

#include <compare>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

namespace fashion_store::domain::shared {

struct ProductId {
    std::string value;
    auto operator<=>(const ProductId&) const = default;
};

struct VariantId {
    std::string value;
    auto operator<=>(const VariantId&) const = default;
};

struct CartId {
    std::string value;
    auto operator<=>(const CartId&) const = default;
};

struct OrderId {
    std::string value;
    auto operator<=>(const OrderId&) const = default;
};

struct OrderItemId {
    std::string value;
    auto operator<=>(const OrderItemId&) const = default;
};

struct CustomerId {
    std::string value;
    auto operator<=>(const CustomerId&) const = default;
};

struct AccountId {
    std::string value;
    auto operator<=>(const AccountId&) const = default;
};

struct EmployeeId {
    std::string value;
    auto operator<=>(const EmployeeId&) const = default;
};

struct ReviewId {
    std::string value;
    auto operator<=>(const ReviewId&) const = default;
};

struct ReturnId {
    std::string value;
    auto operator<=>(const ReturnId&) const = default;
};

struct Size {
    std::string value;
    auto operator<=>(const Size&) const = default;
};

struct Color {
    std::string value;
    auto operator<=>(const Color&) const = default;
};

class Money {
public:
    static Money from_minor(std::int64_t amount) noexcept {
        return Money(amount);
    }

    std::int64_t minor_units() const noexcept {
        return amount_;
    }

    Money operator+(const Money& rhs) const noexcept {
        return Money(amount_ + rhs.amount_);
    }

    Money operator-(const Money& rhs) const noexcept {
        return Money(amount_ - rhs.amount_);
    }

    Money multiply(int quantity) const noexcept {
        return Money(amount_ * quantity);
    }

    auto operator<=>(const Money&) const = default;

private:
    explicit Money(std::int64_t amount) noexcept : amount_(amount) {}

    std::int64_t amount_{0};
};

struct ShippingAddress {
    std::string recipient_name;
    std::string phone;
    std::string line1;
    std::string line2;
    std::string ward;
    std::string district;
    std::string city;
    std::string country;
};

template <typename T, typename E>
class Result {
public:
    static Result ok(T value) {
        return Result(std::move(value));
    }

    static Result fail(E error) {
        return Result(error, FailureTag{});
    }

    bool has_value() const noexcept {
        return std::holds_alternative<T>(state_);
    }

    explicit operator bool() const noexcept {
        return has_value();
    }

    T& value() {
        return std::get<T>(state_);
    }

    const T& value() const {
        return std::get<T>(state_);
    }

    const E& error() const {
        return std::get<E>(state_);
    }

private:
    struct FailureTag {};

    explicit Result(T value) : state_(std::move(value)) {}
    Result(E error, FailureTag) : state_(error) {}

    std::variant<T, E> state_;
};

template <typename E>
using Status = Result<std::monostate, E>;

template <typename E>
inline Status<E> ok_status() {
    return Status<E>::ok(std::monostate{});
}

inline void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::logic_error(message);
    }
}

}  // namespace fashion_store::domain::shared
