# Runtime Data

The C++ HTTP server now writes local demo persistence into this folder at runtime.

Generated files:

- `accounts.txt`
- `customers.txt`
- `products.txt`
- `inventory.txt`
- `carts.txt`
- `orders.txt`
- `vouchers.txt`
- `reviews.txt`
- `returns.txt`

Notes:

- Files are auto-created on first run.
- Seed demo data is only created when the corresponding runtime files are empty.
- Delete the generated `*.txt` files to reset the local backend demo state.
