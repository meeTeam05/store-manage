# Fashion Store System

Single-brand fashion store system with a C++20 core and a luxury-brand support web shell.

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

## Run

```bash
./build/fashion_store
```

## Web Shell

- `web/index.html`: luxury-brand landing page
- `web/product.html`: product detail shell
- `web/cart.html`: cart shell
- `web/login.html`: login shell

Open these files directly in a browser for the current static front-end preview.

## Current Status

- Core domain scaffold is in place
- Application services exist for auth, catalog, cart, customer, order, review, and returns
- In-memory repositories support demo flow
- Smoke test scaffold exists in `tests/core_smoke.cpp`
- Rules and architecture docs are in `rule/` and `docs/`

## Demo Coverage

- account sign-in against in-memory account repository
- customer profile lookup and wishlist update
- cart add, change quantity, remove item support
- checkout preview with voucher validation
- place order, mark paid, and advance lifecycle
- review creation after completed order
- return request creation after completed order
- web login, product, and cart pages share local demo state
