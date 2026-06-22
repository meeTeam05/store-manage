# Fashion Store System

Single-brand fashion store system with a C++20 core and a local support web shell for functional testing.

## Architecture

- `src/domain/`: core business model and business rules
- `src/application/`: use-case orchestration
- `src/infrastructure/`: repository implementations and technical adapters
- `rule/`: project contracts and coding standards
- `web/`: support UI outside the C++ core

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Build With Installed g++

For this repo on Windows, a verified fallback path now exists with MSYS2 `g++`:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-gpp.ps1
```

## Run

```bash
./build/fashion_store
```

For the `g++` fallback build:

```powershell
.\build-gpp\fashion_store.exe
```

## Smoke Test

With CMake:

```bash
ctest --test-dir build
```

With the verified `g++` fallback:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/test-gpp.ps1
```

## Web Shell

- `web/index.html`: local storefront landing page
- `web/product.html`: product detail shell
- `web/cart.html`: cart shell
- `web/login.html`: login shell
- `web/payment.html`: local payment step before order submission
- Visual direction: monochrome black-and-white UI with original-color product/editorial imagery

Open these files directly in a browser for the current static front-end preview.
If a browser keeps an older stylesheet cached, refresh with `Ctrl + F5`.

## Current Status

- Core domain scaffold is in place
- Application services exist for auth, catalog, cart, customer, order, review, returns, staff operations, payment, shipping, reports, and API facade
- Staff-side return management service supports approve, reject, restock, refund, and close flow
- In-memory repositories support demo flow
- File-based repositories support persistent round-trip smoke testing
- Smoke test scaffold exists in `tests/core_smoke.cpp`
- File persistence smoke test exists in `tests/file_persistence_smoke.cpp`
- Order consistency, review/return guard, return management, and report smoke tests cover critical state rules
- Rules and architecture docs are in `rule/` and `docs/`

## Demo Coverage

- account sign-in against in-memory account repository
- customer profile lookup and wishlist update
- cart add, change quantity, remove item support
- checkout preview with voucher validation
- place order, mark paid, and advance lifecycle
- review creation after completed order
- return request creation after completed order
- staff return approval, rejection, restock, refund, and close lifecycle
- revenue, best-selling product, and low-stock report generation
- staff catalog, inventory, order, and voucher management use cases
- catalog product search, filter, and sort use cases
- simulated payment and shipping flows
- API facade for sign-in, catalog search, cart, and checkout flows
- web login, product, cart, and payment pages share local demo state
- web API client can call backend endpoints and falls back to demo state when the API is unavailable
- web storefront uses a dark monochrome shell so the C++ flow demo stays visually consistent across catalog, cart, and payment
