# Memory And Ownership

- Default to value semantics
- Use `std::vector<T>` for owned collections
- Use `std::unique_ptr<T>` for explicit dynamic ownership
- Use `std::shared_ptr<T>` only for real shared lifetime
- Raw pointers and references are non-owning only
