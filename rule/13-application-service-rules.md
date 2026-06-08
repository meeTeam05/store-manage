# Application Service Rules

- Application services orchestrate use-cases
- They load aggregates, call domain behavior, persist results, return typed outcomes
- They must not become manager buckets
- Each public method maps to one use-case
- Business rules stay in entities unless they span multiple aggregates
