#pragma once

#include "domain/shared/types.hpp"

namespace fashion_store::domain::inventory {

using fashion_store::domain::shared::Status;
using fashion_store::domain::shared::VariantId;
using fashion_store::domain::shared::ok_status;
using fashion_store::domain::shared::require;

enum class InventoryError {
    InvalidQuantity,
    InsufficientAvailable,
    InsufficientReserved
};

class InventoryItem {
public:
    InventoryItem(VariantId variant_id, int on_hand, int reserved = 0)
        : variant_id_(std::move(variant_id)), on_hand_(on_hand), reserved_(reserved) {
        require(on_hand_ >= 0, "on hand must be non-negative");
        require(reserved_ >= 0, "reserved must be non-negative");
        require(reserved_ <= on_hand_, "reserved cannot exceed on hand");
    }

    const VariantId& variant_id() const noexcept { return variant_id_; }
    int on_hand() const noexcept { return on_hand_; }
    int reserved() const noexcept { return reserved_; }
    int available() const noexcept { return on_hand_ - reserved_; }

    Status<InventoryError> reserve(int quantity) {
        if (quantity <= 0) {
            return Status<InventoryError>::fail(InventoryError::InvalidQuantity);
        }
        if (quantity > available()) {
            return Status<InventoryError>::fail(InventoryError::InsufficientAvailable);
        }
        reserved_ += quantity;
        return ok_status<InventoryError>();
    }

    Status<InventoryError> release(int quantity) {
        if (quantity <= 0) {
            return Status<InventoryError>::fail(InventoryError::InvalidQuantity);
        }
        if (quantity > reserved_) {
            return Status<InventoryError>::fail(InventoryError::InsufficientReserved);
        }
        reserved_ -= quantity;
        return ok_status<InventoryError>();
    }

    Status<InventoryError> commit_sale(int quantity) {
        if (quantity <= 0) {
            return Status<InventoryError>::fail(InventoryError::InvalidQuantity);
        }
        if (quantity > reserved_) {
            return Status<InventoryError>::fail(InventoryError::InsufficientReserved);
        }
        reserved_ -= quantity;
        on_hand_ -= quantity;
        return ok_status<InventoryError>();
    }

    Status<InventoryError> restock(int quantity) {
        if (quantity <= 0) {
            return Status<InventoryError>::fail(InventoryError::InvalidQuantity);
        }
        on_hand_ += quantity;
        return ok_status<InventoryError>();
    }

private:
    VariantId variant_id_;
    int on_hand_;
    int reserved_;
};

}  // namespace fashion_store::domain::inventory
