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
  - order
- `src/infrastructure/`
  - in-memory persistence scaffold
- `web/`
  - luxury storefront shell

## Current State

- Domain objects and repository contracts exist
- Application services for catalog, cart, customer, and order exist
- In-memory repositories exist for local demo flow
- `src/main.cpp` seeds sample data and runs one purchase flow
- `web/` contains placeholder luxury brand screens

## Next Recommended Work

1. Add `.cpp` implementations and move logic out of headers where appropriate
2. Add tests for money, inventory, cart, order state transitions, and voucher validation
3. Add real persistence adapter
4. Connect web login and storefront to application/backend
