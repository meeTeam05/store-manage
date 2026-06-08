# Project Structure

- `src/domain/`: entities, value objects, aggregates, domain services, repository interfaces
- `src/application/`: application services and use-case orchestration
- `src/infrastructure/`: persistence and technical adapters
- `web/`: support UI only
- `docs/`: requirements and design notes
- `rule/`: mandatory team contracts

## Rules

- Organize domain by business capability, not by artifact type
- Keep repository interfaces inside owning domain module
- Keep technical code out of domain
- Avoid catch-all folders like `utils` or `misc`
