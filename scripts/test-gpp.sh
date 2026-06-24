#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_SCRIPT="$SCRIPT_DIR/build-gpp.sh"

# Build first
bash "$BUILD_SCRIPT"

# Run all smoke tests
echo ""
echo "Running smoke tests..."
echo "======================"

"$ROOT/build-gpp/fashion_store_core_smoke"
echo "✓ Core smoke test passed"

"$ROOT/build-gpp/fashion_store_file_persistence_smoke"
echo "✓ File persistence smoke test passed"

"$ROOT/build-gpp/fashion_store_order_consistency_smoke"
echo "✓ Order consistency smoke test passed"

"$ROOT/build-gpp/fashion_store_review_return_guards_smoke"
echo "✓ Review return guards smoke test passed"

"$ROOT/build-gpp/fashion_store_return_management_smoke"
echo "✓ Return management smoke test passed"

"$ROOT/build-gpp/fashion_store_report_smoke"
echo "✓ Report smoke test passed"

"$ROOT/build-gpp/fashion_store_staff_catalog_management_smoke"
echo "✓ Staff catalog management smoke test passed"

"$ROOT/build-gpp/fashion_store_payment_smoke"
echo "✓ Payment smoke test passed"

"$ROOT/build-gpp/fashion_store_shipping_smoke"
echo "✓ Shipping smoke test passed"

"$ROOT/build-gpp/fashion_store_api_facade_smoke"
echo "✓ API facade smoke test passed"

"$ROOT/build-gpp/fashion_store_auth_roles_smoke"
echo "✓ Auth roles smoke test passed"

echo ""
echo "======================"
echo "All smoke tests passed ✓"
