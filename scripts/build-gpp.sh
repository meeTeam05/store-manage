#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="$ROOT/build-gpp"
COMPILER="g++"

if ! command -v "$COMPILER" &> /dev/null; then
    echo "Error: g++ not found"
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

CORE_SOURCES=(
    "src/domain/catalog/product.cpp"
    "src/domain/inventory/inventory_item.cpp"
    "src/domain/cart/cart.cpp"
    "src/domain/order/order.cpp"
    "src/domain/pricing/voucher.cpp"
    "src/domain/customer/customer.cpp"
    "src/domain/identity/account.cpp"
    "src/infrastructure/persistence/file/file_store.cpp"
    "src/infrastructure/persistence/file/file_repositories.cpp"
)

cd "$ROOT"

echo "Building main executable..."
$COMPILER -std=c++20 -Isrc src/main.cpp "${CORE_SOURCES[@]}" -o "$OUTPUT_DIR/fashion_store"

echo "Building core smoke test..."
$COMPILER -std=c++20 -Isrc tests/core_smoke.cpp "${CORE_SOURCES[@]}" -o "$OUTPUT_DIR/fashion_store_core_smoke"

echo "Building file persistence smoke test..."
$COMPILER -std=c++20 -Isrc tests/file_persistence_smoke.cpp "${CORE_SOURCES[@]}" -o "$OUTPUT_DIR/fashion_store_file_persistence_smoke"

echo "Building order consistency smoke test..."
$COMPILER -std=c++20 -Isrc tests/order_consistency_smoke.cpp "${CORE_SOURCES[@]}" -o "$OUTPUT_DIR/fashion_store_order_consistency_smoke"

echo "Building review return guards smoke test..."
$COMPILER -std=c++20 -Isrc tests/review_return_guards_smoke.cpp "${CORE_SOURCES[@]}" -o "$OUTPUT_DIR/fashion_store_review_return_guards_smoke"

echo "Building return management smoke test..."
$COMPILER -std=c++20 -Isrc tests/return_management_smoke.cpp "${CORE_SOURCES[@]}" -o "$OUTPUT_DIR/fashion_store_return_management_smoke"

echo "Building report smoke test..."
$COMPILER -std=c++20 -Isrc tests/report_smoke.cpp "${CORE_SOURCES[@]}" -o "$OUTPUT_DIR/fashion_store_report_smoke"

echo "Building staff catalog management smoke test..."
$COMPILER -std=c++20 -Isrc tests/staff_catalog_management_smoke.cpp "${CORE_SOURCES[@]}" -o "$OUTPUT_DIR/fashion_store_staff_catalog_management_smoke"

echo "Building payment smoke test..."
$COMPILER -std=c++20 -Isrc tests/payment_smoke.cpp "${CORE_SOURCES[@]}" -o "$OUTPUT_DIR/fashion_store_payment_smoke"

echo "Building shipping smoke test..."
$COMPILER -std=c++20 -Isrc tests/shipping_smoke.cpp "${CORE_SOURCES[@]}" -o "$OUTPUT_DIR/fashion_store_shipping_smoke"

echo "Building api facade smoke test..."
$COMPILER -std=c++20 -Isrc tests/api_facade_smoke.cpp "${CORE_SOURCES[@]}" -o "$OUTPUT_DIR/fashion_store_api_facade_smoke"

echo "Building auth roles smoke test..."
$COMPILER -std=c++20 -Isrc tests/auth_roles_smoke.cpp "${CORE_SOURCES[@]}" -o "$OUTPUT_DIR/fashion_store_auth_roles_smoke"

echo "Build completed in $OUTPUT_DIR"
