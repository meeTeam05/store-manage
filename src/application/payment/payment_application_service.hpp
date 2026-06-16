#pragma once

#include <memory>

#include "domain/order/order.hpp"
#include "domain/payment/payment.hpp"

namespace fashion_store::application::payment {

using fashion_store::domain::order::Order;
using fashion_store::domain::order::PaymentMethod;
using fashion_store::domain::payment::BankTransferPayment;
using fashion_store::domain::payment::CashPayment;
using fashion_store::domain::payment::EWalletPayment;
using fashion_store::domain::payment::Payment;
using fashion_store::domain::payment::PaymentError;
using fashion_store::domain::payment::PaymentReceipt;
using fashion_store::domain::shared::Result;

class PaymentApplicationService {
public:
    Result<PaymentReceipt, PaymentError> authorize_order_payment(const Order& order) const {
        auto payment = create_payment(order);
        if (!payment) {
            return Result<PaymentReceipt, PaymentError>::fail(PaymentError::UnsupportedMethod);
        }
        return payment->authorize();
    }

private:
    static std::unique_ptr<Payment> create_payment(const Order& order) {
        switch (order.payment_method()) {
            case PaymentMethod::Cash:
                return std::make_unique<CashPayment>(order.id(), order.total());
            case PaymentMethod::BankTransfer:
                return std::make_unique<BankTransferPayment>(order.id(), order.total());
            case PaymentMethod::EWallet:
                return std::make_unique<EWalletPayment>(order.id(), order.total());
        }
        return nullptr;
    }
};

}  // namespace fashion_store::application::payment
