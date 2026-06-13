#pragma once

#include <optional>

#include "domain/inventory/inventory_item.hpp"

namespace fashion_store::domain::inventory {

class IInventoryRepository {
public:
    virtual ~IInventoryRepository() = default;

    virtual std::optional<InventoryItem> find_by_variant_id(const VariantId& variant_id) const = 0;
    virtual void save(const InventoryItem& item) = 0;
};

}  // namespace fashion_store::domain::inventory
