#pragma once

#include <chrono>
#include <optional>
#include <string>

#include "domain/shared/types.hpp"

namespace fashion_store::domain::pricing {

using fashion_store::domain::shared::Money;
using fashion_store::domain::shared::Status;
using fashion_store::domain::shared::ok_status;
using fashion_store::domain::shared::require;

enum class DiscountType {
    Percentage,
    FixedAmount
};

enum class VoucherError {
    Inactive,
    OutOfWindow,
    MinimumOrderNotMet,
    UsageLimitExceeded
};

class Voucher {
public:
    Voucher(std::string code,
            DiscountType discount_type,
            int discount_value,
            Money min_order_amount,
            std::optional<Money> max_discount_amount,
            std::chrono::system_clock::time_point start_at,
            std::chrono::system_clock::time_point end_at,
            std::optional<int> remaining_uses,
            bool is_active);

    const std::string& code() const noexcept { return code_; }
    DiscountType discount_type() const noexcept { return discount_type_; }
    int discount_value() const noexcept { return discount_value_; }
    Money min_order_amount() const noexcept { return min_order_amount_; }
    const std::optional<Money>& max_discount_amount() const noexcept { return max_discount_amount_; }
    std::chrono::system_clock::time_point start_at() const noexcept { return start_at_; }
    std::chrono::system_clock::time_point end_at() const noexcept { return end_at_; }
    const std::optional<int>& remaining_uses() const noexcept { return remaining_uses_; }
    bool is_active() const noexcept { return is_active_; }

    Status<VoucherError> validate(Money subtotal, std::chrono::system_clock::time_point now) const;

    Money calculate_discount(Money subtotal) const;

    Status<VoucherError> consume();

    void restore_use() noexcept;

private:
    std::string code_;
    DiscountType discount_type_;
    int discount_value_;
    Money min_order_amount_;
    std::optional<Money> max_discount_amount_;
    std::chrono::system_clock::time_point start_at_;
    std::chrono::system_clock::time_point end_at_;
    std::optional<int> remaining_uses_;
    bool is_active_;
};

}  // namespace fashion_store::domain::pricing
