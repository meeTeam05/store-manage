# Architecture Overview

## Core Direction

- Core language: C++20
- Core scope: domain and business rules
- Model style: entity-rich
- Project type: single-brand fashion store

## Layers

- `src/domain/`
  - catalog
  - inventory
  - cart
  - order
  - pricing
  - identity
  - customer
  - staff
  - review
  - returns
  - shared
- `src/application/`
  - catalog
  - cart
  - customer
  - identity
  - order
  - review
  - returns
  - staff
  - report
  - payment
  - shipping
- `src/api/`
  - facade for backend/API adapters
- `src/infrastructure/`
  - in-memory persistence scaffold
  - file persistence scaffold
- `web/`
  - local storefront shell for functional testing

## Current State

- Domain objects and repository contracts exist
- Application services for catalog, cart, customer, and order exist
- Application services for identity, review, returns, and staff return management exist
- Authentication and customer profile flows exist through application services
- In-memory repositories exist for local demo flow
- File persistence adapters exist for the current domain repositories
- Return request lifecycle supports approve, reject, restock, refund, and close
- Report application service supports revenue, best-selling products, and low-stock inventory summaries
- Staff operation service supports catalog, inventory, order, and voucher management use cases
- Catalog application service supports search, filter, and sort
- Payment and shipping modules provide simulated operational flows
- API facade exposes sign-in, catalog search, cart, and checkout use cases for future HTTP adapters
- `src/main.cpp` seeds sample data and runs one purchase flow
- `web/` contains linked storefront pages for login, catalog, cart, and payment backed by shared demo state with API-client fallback support
- Fallback Windows build and smoke-test scripts exist under `scripts/`

## Next Recommended Work

1. Add real HTTP server adapter around `src/api/FashionStoreApi`
2. Connect web payment submission, checkout, and cart refresh to live backend responses
3. Replace file persistence with SQLite or another real database adapter if required
4. Add deeper unit tests for invalid payment, shipping, and admin edge cases
