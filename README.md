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
- Application services exist for catalog, cart, customer, and order
- In-memory repositories support demo flow
- Smoke test scaffold exists in `tests/core_smoke.cpp`
- Rules and architecture docs are in `rule/` and `docs/`
