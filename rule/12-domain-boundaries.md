# Domain Boundaries

## Catalog
- Owns products and variants
- Does not own stock

## Inventory
- Owns `on_hand`, `reserved`, and stock rules

## Cart
- Owns buyer intent before order placement

## Order
- Owns transaction history, payment state, and shipping snapshot

## Pricing
- Owns voucher validation and order-level discount

## Identity
- Owns login credentials and role only
