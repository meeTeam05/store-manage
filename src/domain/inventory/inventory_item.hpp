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
    InventoryItem(VariantId variant_id, int on_hand, int reserved = 0);

    const VariantId& variant_id() const noexcept { return variant_id_; }
    int on_hand() const noexcept { return on_hand_; }
    int reserved() const noexcept { return reserved_; }
    int available() const noexcept { return on_hand_ - reserved_; }

    Status<InventoryError> reserve(int quantity);

    Status<InventoryError> release(int quantity);

    Status<InventoryError> commit_sale(int quantity);

    Status<InventoryError> restock(int quantity);

private:
    VariantId variant_id_;
    int on_hand_;
    int reserved_;
};

}  // namespace fashion_store::domain::inventory
