# Project Scope And Architecture

- Core language: C++20
- Core scope: `domain + business rules`
- Architecture choice: `A + A1 + App2`
- Domain model: entity-rich
- Project type: single-brand fashion store
- `Product` is aggregate root
- `ProductVariant` is child entity
- `InventoryItem` is separate aggregate keyed by `variant_id`
- `Cart` stores price snapshots
- `Order` stores immutable purchase snapshots
- `application/` orchestrates use-cases
- `infrastructure/` implements technical details
