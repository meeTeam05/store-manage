#include "domain/inventory/inventory_item.hpp"

namespace fashion_store::domain::inventory {

InventoryItem::InventoryItem(VariantId variant_id, int on_hand, int reserved)
    : variant_id_(std::move(variant_id)), on_hand_(on_hand), reserved_(reserved) {
    require(on_hand_ >= 0, "on hand must be non-negative");
    require(reserved_ >= 0, "reserved must be non-negative");
    require(reserved_ <= on_hand_, "reserved cannot exceed on hand");
}

Status<InventoryError> InventoryItem::reserve(int quantity) {
    if (quantity <= 0) {
        return Status<InventoryError>::fail(InventoryError::InvalidQuantity);
    }
    if (quantity > available()) {
        return Status<InventoryError>::fail(InventoryError::InsufficientAvailable);
    }
    reserved_ += quantity;
    return ok_status<InventoryError>();
}

Status<InventoryError> InventoryItem::release(int quantity) {
    if (quantity <= 0) {
        return Status<InventoryError>::fail(InventoryError::InvalidQuantity);
    }
    if (quantity > reserved_) {
        return Status<InventoryError>::fail(InventoryError::InsufficientReserved);
    }
    reserved_ -= quantity;
    return ok_status<InventoryError>();
}

Status<InventoryError> InventoryItem::commit_sale(int quantity) {
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

Status<InventoryError> InventoryItem::restock(int quantity) {
    if (quantity <= 0) {
        return Status<InventoryError>::fail(InventoryError::InvalidQuantity);
    }
    on_hand_ += quantity;
    return ok_status<InventoryError>();
}

}  // namespace fashion_store::domain::inventory
