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
- `src/infrastructure/`
  - in-memory persistence scaffold
  - file persistence scaffold
- `web/`
  - luxury storefront shell

## Current State

- Domain objects and repository contracts exist
- Application services for catalog, cart, customer, and order exist
- Application services for identity, review, returns, and staff return management exist
- Authentication and customer profile flows exist through application services
- In-memory repositories exist for local demo flow
- File persistence adapters exist for the current domain repositories
- Return request lifecycle supports approve, reject, restock, refund, and close
- Report application service supports revenue, best-selling products, and low-stock inventory summaries
- `src/main.cpp` seeds sample data and runs one purchase flow
- `web/` contains linked storefront pages backed by shared demo state
- Fallback Windows build and smoke-test scripts exist under `scripts/`

## Next Recommended Work

1. Add staff/admin services for catalog, inventory, order, and voucher management
2. Add backend/API adapter for the C++ core
3. Connect web login and storefront to the backend/API
4. Replace file persistence with SQLite or another real database adapter if required
