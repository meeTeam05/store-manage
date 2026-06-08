# Dependency Direction

- `domain` depends only on standard library and allowed domain modules
- `application` may depend on `domain`
- `infrastructure` may depend on `domain` and `application`
- `web` may depend on `application`
- `domain` must not depend on `application`, `infrastructure`, or `web`

## Boundaries

- `catalog` does not own inventory
- `inventory` does not own product metadata
- `order` stores history snapshots
- `review` may validate against completed orders but does not mutate them
