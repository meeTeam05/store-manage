# Error Handling

- Expected business failures use `Result`
- Exceptions are for infrastructure failure, broken contract, or impossible state
- Do not use exceptions for normal business flow
- Error enums belong to the owning module
