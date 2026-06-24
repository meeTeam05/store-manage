# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and Test

```bash
# Configure and build
cmake -S . -B build
cmake --build build

# Run the main demo
./build/fashion_store

# Run all smoke tests
ctest --test-dir build

# Run a single smoke test binary directly
./build/fashion_store_core_smoke
./build/fashion_store_order_consistency_smoke
./build/fashion_store_review_return_guards_smoke
./build/fashion_store_return_management_smoke
./build/fashion_store_report_smoke
./build/fashion_store_staff_catalog_management_smoke
./build/fashion_store_payment_smoke
./build/fashion_store_shipping_smoke
./build/fashion_store_api_facade_smoke
./build/fashion_store_auth_roles_smoke
./build/fashion_store_file_persistence_smoke

# Synchronize mock data from C++ JSON seeds to Frontend storefront-data.js
npm run sync # or node scripts/sync-demo-data.js

# Start frontend dev server
npm run dev

```

Windows fallback (MSYS2 g++):
```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-gpp.ps1
powershell -ExecutionPolicy Bypass -File scripts/test-gpp.ps1
```

## Architecture

The project is a C++20 domain-driven fashion store. The dependency rule is strict: `domain` -> `application` -> `infrastructure`/`api`. Domain must never import from application or infrastructure.

**Layers:**

- `src/domain/` -- aggregate roots, entities, value objects, repository interfaces. No external dependencies beyond stdlib. Each subdirectory owns one bounded context.
- `src/application/` -- one service per use-case group (e.g. `CartApplicationService`, `OrderApplicationService`). Each public method = one use case. Services load aggregates, invoke domain behavior, persist results.
- `src/infrastructure/persistence/` -- two adapters: `in_memory/` for demo/test, `file/` for round-trip smoke testing. Both implement the repository interfaces from `domain/`.
- `src/api/fashion_store_api.hpp` -- thin facade composing application services; intended as the seam for a future HTTP adapter.
- `web/` -- static HTML pages (login, catalog, cart, payment) backed by shared `localStorage` demo state; falls back to local state when no backend is running.

**Key domain types** (all in `src/domain/shared/types.hpp`):

- `Money` -- stores in minor units (`int64_t`); never use `double` for money.
- `Result<T, E>` / `Status<E>` -- used for expected business failures. Exceptions are reserved for infrastructure failures or broken invariants only.
- `require(bool, message)` -- throws `std::logic_error` for impossible states / precondition violations inside domain objects.

**Aggregate boundaries:**

- `Product` owns `ProductVariant`; `InventoryItem` is a separate aggregate keyed by `variant_id`
- `Cart` stores price snapshots (not live prices); `Order` stores immutable purchase snapshots
- `ReturnRequest` targets one `OrderItem`; `Review` may read completed orders but never mutates them

## Coding Rules (from `rule/`)

- C++20: use `enum class`, `constexpr`, `std::optional`, `std::string_view` (only when lifetime is safe)
- No `double` for money; no C-style casts; no macros unless integration requires them
- Memory: value semantics by default; `unique_ptr` for explicit dynamic ownership; `shared_ptr` only for genuinely shared lifetime; raw pointers/references are non-owning
- Naming: types `PascalCase`, functions/variables `snake_case`, private members `snake_case_`, interfaces prefixed `I`
- Headers: no `using namespace` in headers; prefer forward declarations; put heavy includes in `.cpp`
- Business rules live in the closest owning aggregate; cross-aggregate rules go to a domain service or application service

## Adding a New Smoke Test

1. Create `tests/<name>_smoke.cpp` with a `main()` that returns 0 on success / non-zero on failure.
2. Add to `CMakeLists.txt`:
   ```cmake
   add_executable(fashion_store_<name>_smoke tests/<name>_smoke.cpp)
   target_link_libraries(fashion_store_<name>_smoke PRIVATE fashion_store_core)
   add_test(NAME fashion_store_<name>_smoke COMMAND fashion_store_<name>_smoke)
   ```
