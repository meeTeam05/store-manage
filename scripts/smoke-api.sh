#!/usr/bin/env bash
# Smoke test: hit every API endpoint. Exits 0 on all-pass, 1 on any failure.
set -euo pipefail

BASE="http://localhost:8080"
PASS=0
FAIL=0

check() {
  local label="$1"
  local response="$2"
  if echo "$response" | grep -q '"ok":true'; then
    echo "  PASS  $label"
    PASS=$((PASS + 1))
  else
    echo "  FAIL  $label -> $response"
    FAIL=$((FAIL + 1))
  fi
}

POST() {
  local path="$1"; local body="${2:-{}}"
  curl -s -X POST "$BASE$path" -H "Content-Type: application/json" -d "$body"
}

GET() {
  local path="$1"
  curl -s "$BASE$path"
}

echo "=== Auth ==="
SIGN_IN=$(POST /api/sign-in '{"username":"client001","password_hash":"hash:123456"}')
check "POST /api/sign-in (customer)"      "$SIGN_IN"
STAFF_IN=$(POST /api/sign-in '{"username":"staff001","password_hash":"hash:123456"}')
check "POST /api/sign-in (staff)"         "$STAFF_IN"

echo "=== Catalog ==="
check "GET /api/products"                 "$(GET /api/products)"
check "GET /api/products?category=Dresses" "$(GET '/api/products?category=Dresses')"
check "GET /api/products/product-001"     "$(GET /api/products/product-001)"
check "GET /api/variants/variant-001-black-m" "$(GET /api/variants/variant-001-black-m)"
check "GET /api/products/product-001/reviews" "$(GET /api/products/product-001/reviews)"

echo "=== Cart ==="
CART=$(POST /api/cart/add '{"cart_id":"cart-test-01","customer_id":"customer-001","variant_id":"variant-001-white-l","quantity":2}')
check "POST /api/cart/add"                "$CART"
check "POST /api/cart/quantity"           "$(POST /api/cart/quantity '{"cart_id":"cart-test-01","variant_id":"variant-001-white-l","quantity":1}')"
check "POST /api/cart/remove"             "$(POST /api/cart/remove '{"cart_id":"cart-test-01","variant_id":"variant-001-white-l"}')"

echo "=== Checkout ==="
# Re-add item so checkout has something
POST /api/cart/add '{"cart_id":"cart-test-02","customer_id":"customer-001","variant_id":"variant-003-olive-l","quantity":1}' > /dev/null
check "POST /api/checkout/preview"        "$(POST /api/checkout/preview '{"cart_id":"cart-test-02","voucher_code":"WELCOME10"}')"
ORDER=$(POST /api/checkout '{"order_id":"order-api-test","cart_id":"cart-test-02","customer_id":"customer-001","method":"Cash","recipient_name":"Test","phone":"09","line1":"1 Street","line2":"","ward":"W","district":"D","city":"HCM","country":"VN"}')
check "POST /api/checkout"                "$ORDER"

echo "=== Orders ==="
check "GET /api/orders/order-api-test"    "$(GET /api/orders/order-api-test)"
check "GET /api/customers/customer-001/orders" "$(GET /api/customers/customer-001/orders)"
check "POST /api/orders/order-api-test/pay"    "$(POST /api/orders/order-api-test/pay)"
check "POST /api/orders/order-api-test/cancel" "$(POST /api/orders/order-api-test/cancel)"

echo "=== Reviews ==="
check "POST /api/reviews"                 "$(POST /api/reviews '{"order_id":"order-seed-001","review_id":"review-smoke-01","customer_id":"customer-001","product_id":"product-001","variant_id":"variant-001-black-m","rating":5,"comment":"Great!"}')"
check "GET /api/products/product-001/reviews" "$(GET /api/products/product-001/reviews)"
check "GET /api/customers/customer-001/reviews" "$(GET /api/customers/customer-001/reviews)"

echo "=== Returns ==="
ORDER_ITEMS=$(GET /api/orders/order-seed-001)
ITEM_ID=$(echo "$ORDER_ITEMS" | grep -o '"order_item_id":"[^"]*"' | head -1 | cut -d'"' -f4)
check "POST /api/returns"                 "$(POST /api/returns "{\"order_id\":\"order-seed-001\",\"return_id\":\"return-smoke-01\",\"order_item_id\":\"$ITEM_ID\",\"quantity\":1,\"reason\":\"Wrong size\"}")"
check "GET /api/orders/order-seed-001/returns" "$(GET /api/orders/order-seed-001/returns)"
check "POST /api/returns/return-smoke-01/approve" "$(POST /api/returns/return-smoke-01/approve)"
check "POST /api/returns/return-smoke-01/restock"  "$(POST /api/returns/return-smoke-01/restock)"
check "POST /api/returns/return-smoke-01/refund"   "$(POST /api/returns/return-smoke-01/refund)"
check "POST /api/returns/return-smoke-01/close"    "$(POST /api/returns/return-smoke-01/close)"

echo "=== Staff ==="
check "POST /api/staff/products"          "$(POST /api/staff/products '{"product_id":"product-staff-01","name":"Linen Shirt","category":"Tops","description":"Breathable linen.","collection":"SS2026","status":"Active"}')"
check "POST /api/staff/products/product-staff-01/status" "$(POST /api/staff/products/product-staff-01/status '{"status":"Active"}')"
check "POST /api/staff/products/product-staff-01/variants" "$(POST /api/staff/products/product-staff-01/variants '{"variant_id":"var-staff-01","sku":"LN-S-01","size":"S","color":"White","price_minor":280000}')"
check "POST /api/staff/inventory"         "$(POST /api/staff/inventory '{"variant_id":"var-staff-01","on_hand":30,"reserved":0}')"
check "POST /api/staff/inventory/var-staff-01/restock" "$(POST /api/staff/inventory/var-staff-01/restock '{"quantity":10}')"
check "POST /api/staff/vouchers"          "$(POST /api/staff/vouchers '{"code":"SUMMER20","rate_percent":20,"min_order_minor":500000,"max_discount_minor":120000,"max_uses":50}')"
check "GET /api/staff/orders"             "$(GET /api/staff/orders)"
# advance order-seed-002 (Paid) → Packed via staff
check "POST /api/staff/orders/order-seed-002/advance" "$(POST /api/staff/orders/order-seed-002/advance '{"status":"Packed"}')"
# create a fresh Paid order specifically for staff cancel test
POST /api/cart/add '{"cart_id":"cart-sc","customer_id":"customer-001","variant_id":"variant-003-olive-l","quantity":1}' > /dev/null
POST /api/checkout '{"order_id":"order-sc","cart_id":"cart-sc","customer_id":"customer-001","method":"Cash","recipient_name":"Test","phone":"09","line1":"1","line2":"","ward":"W","district":"D","city":"HCM","country":"VN"}' > /dev/null
POST /api/orders/order-sc/pay > /dev/null
check "POST /api/staff/orders/order-sc/cancel" "$(POST /api/staff/orders/order-sc/cancel)"

echo "=== Payment ==="
check "POST /api/payment/authorize"       "$(POST /api/payment/authorize '{"order_id":"order-seed-001"}')"

echo "=== Shipping ==="
check "GET /api/shipping/quote"           "$(GET /api/shipping/quote)"
check "GET /api/shipping/quote?method=Express" "$(GET '/api/shipping/quote?method=Express')"
check "POST /api/shipping/shipments"      "$(POST /api/shipping/shipments '{"order_id":"order-seed-001","method":"Standard"}')"

echo "=== Reports ==="
check "GET /api/reports/revenue"          "$(GET /api/reports/revenue)"
check "GET /api/reports/best-selling"     "$(GET /api/reports/best-selling)"
check "GET /api/reports/low-stock"        "$(GET /api/reports/low-stock)"
check "GET /api/reports/low-stock?threshold=50" "$(GET '/api/reports/low-stock?threshold=50')"

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
