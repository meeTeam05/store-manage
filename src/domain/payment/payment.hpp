#pragma once

#include <string>
#include <utility>

#include "domain/order/order.hpp"
#include "domain/shared/types.hpp"

namespace fashion_store::domain::payment {

using fashion_store::domain::order::PaymentMethod;
using fashion_store::domain::shared::Money;
using fashion_store::domain::shared::OrderId;
using fashion_store::domain::shared::Result;

enum class PaymentError {
    InvalidAmount,
    UnsupportedMethod
};

struct PaymentReceipt {
    OrderId order_id;
    PaymentMethod method;
    Money amount;
    std::string reference;
};

class Payment {
public:
    Payment(OrderId order_id, Money amount)
        : order_id_(std::move(order_id)), amount_(amount) {}

    virtual ~Payment() = default;

    const OrderId& order_id() const noexcept { return order_id_; }
    Money amount() const noexcept { return amount_; }

    virtual PaymentMethod method() const noexcept = 0;
    virtual std::string reference_prefix() const = 0;

    Result<PaymentReceipt, PaymentError> authorize() const {
        if (amount_ <= Money::from_minor(0)) {
            return Result<PaymentReceipt, PaymentError>::fail(PaymentError::InvalidAmount);
        }
        return Result<PaymentReceipt, PaymentError>::ok(PaymentReceipt{
            order_id_,
            method(),
            amount_,
            reference_prefix() + "-" + order_id_.value
        });
    }

private:
    OrderId order_id_;
    Money amount_;
};

class CashPayment final : public Payment {
public:
    using Payment::Payment;

    PaymentMethod method() const noexcept override { return PaymentMethod::Cash; }
    std::string reference_prefix() const override { return "CASH"; }
};

class BankTransferPayment final : public Payment {
public:
    using Payment::Payment;

    PaymentMethod method() const noexcept override { return PaymentMethod::BankTransfer; }
    std::string reference_prefix() const override { return "BANK"; }
};

class EWalletPayment final : public Payment {
public:
    using Payment::Payment;

    PaymentMethod method() const noexcept override { return PaymentMethod::EWallet; }
    std::string reference_prefix() const override { return "EWALLET"; }
};

}  // namespace fashion_store::domain::payment
