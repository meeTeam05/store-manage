# Domain Modeling Rules

- Model real business concepts first
- Prefer entities with behavior over passive structs
- Use value objects for ids, money, size, color, and address
- Every aggregate root owns its child invariants
- Business rules stay in the closest owning object
- Cross-aggregate rules go to domain service or application service
- Repository interfaces are domain contracts

## Aggregate Choices

- `Product` owns `ProductVariant`
- `Cart` owns `CartItem`
- `Order` owns `OrderItem`
- `InventoryItem` stands alone per `variant_id`
- `ReturnRequest` targets one `OrderItem`
