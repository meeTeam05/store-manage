#include "domain/pricing/voucher.hpp"

namespace fashion_store::domain::pricing {

Voucher::Voucher(std::string code,
                 DiscountType discount_type,
                 int discount_value,
                 Money min_order_amount,
                 std::optional<Money> max_discount_amount,
                 std::chrono::system_clock::time_point start_at,
                 std::chrono::system_clock::time_point end_at,
                 std::optional<int> remaining_uses,
                 bool is_active)
    : code_(std::move(code)),
      discount_type_(discount_type),
      discount_value_(discount_value),
      min_order_amount_(min_order_amount),
      max_discount_amount_(max_discount_amount),
      start_at_(start_at),
      end_at_(end_at),
      remaining_uses_(remaining_uses),
      is_active_(is_active) {
    require(!code_.empty(), "voucher code must not be empty");
    require(start_at_ <= end_at_, "voucher window must be valid");
    require(discount_value_ >= 0, "discount value must be non-negative");
}

Status<VoucherError> Voucher::validate(Money subtotal, std::chrono::system_clock::time_point now) const {
    if (!is_active_) {
        return Status<VoucherError>::fail(VoucherError::Inactive);
    }
    if (now < start_at_ || now > end_at_) {
        return Status<VoucherError>::fail(VoucherError::OutOfWindow);
    }
    if (subtotal < min_order_amount_) {
        return Status<VoucherError>::fail(VoucherError::MinimumOrderNotMet);
    }
    if (remaining_uses_.has_value() && *remaining_uses_ <= 0) {
        return Status<VoucherError>::fail(VoucherError::UsageLimitExceeded);
    }
    return ok_status<VoucherError>();
}

Money Voucher::calculate_discount(Money subtotal) const {
    auto discount = Money::from_minor(0);
    if (discount_type_ == DiscountType::FixedAmount) {
        discount = Money::from_minor(discount_value_);
    } else {
        discount = Money::from_minor((subtotal.minor_units() * discount_value_) / 100);
    }
    if (max_discount_amount_.has_value() && discount > *max_discount_amount_) {
        discount = *max_discount_amount_;
    }
    if (discount > subtotal) {
        discount = subtotal;
    }
    return discount;
}

Status<VoucherError> Voucher::consume() {
    if (remaining_uses_.has_value()) {
        if (*remaining_uses_ <= 0) {
            return Status<VoucherError>::fail(VoucherError::UsageLimitExceeded);
        }
        --(*remaining_uses_);
    }
    return ok_status<VoucherError>();
}

void Voucher::restore_use() noexcept {
    if (remaining_uses_.has_value()) {
        ++(*remaining_uses_);
    }
}

}  // namespace fashion_store::domain::pricing
