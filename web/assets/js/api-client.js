(function attachStorefrontApi() {
  const baseUrl = window.STOREFRONT_API_BASE_URL || "";

  async function request(path, options = {}) {
    try {
      const response = await fetch(`${baseUrl}${path}`, {
        headers: { "Content-Type": "application/json", ...(options.headers || {}) },
        ...options
      });
      const body = await response.json();
      if (!response.ok) {
        return { ok: false, error: body.error || `HTTP ${response.status}` };
      }
      return { ok: true, data: body.data !== undefined ? body.data : body };
    } catch {
      return { ok: false, error: "API unavailable" };
    }
  }

  window.storefrontApi = {
    // Auth
    async signIn(username, passwordHash) {
      return request("/api/sign-in", {
        method: "POST",
        body: JSON.stringify({ username, password_hash: passwordHash })
      });
    },
    async registerCustomer({ username, passwordHash, fullName, phone, address = {} }) {
      return request("/api/register", {
        method: "POST",
        body: JSON.stringify({
          username,
          password_hash: passwordHash,
          full_name: fullName,
          phone,
          recipient_name: address.recipientName || fullName,
          line1: address.line1 || "12 Nguyen Hue",
          line2: address.line2 || "",
          ward: address.ward || "Ben Nghe",
          district: address.district || "District 1",
          city: address.city || "Ho Chi Minh City",
          country: address.country || "Vietnam"
        })
      });
    },

    // Catalog
    async getProducts(params = {}) {
      const qs = new URLSearchParams(params).toString();
      return request(`/api/products${qs ? "?" + qs : ""}`);
    },
    async getProduct(productId) {
      return request(`/api/products/${productId}`);
    },
    async getVariant(variantId) {
      return request(`/api/variants/${variantId}`);
    },
    async getProductReviews(productId) {
      return request(`/api/products/${productId}/reviews`);
    },
    async getCustomerProfile(customerId) {
      return request(`/api/customers/${customerId}`);
    },
    async addToWishlist(customerId, productId) {
      return request(`/api/customers/${customerId}/wishlist`, {
        method: "POST",
        body: JSON.stringify({ product_id: productId })
      });
    },
    async removeFromWishlist(customerId, productId) {
      return request(`/api/customers/${customerId}/wishlist/remove`, {
        method: "POST",
        body: JSON.stringify({ product_id: productId })
      });
    },

    // Cart
    async addToCart(cartId, customerId, variantId, quantity) {
      return request("/api/cart/add", {
        method: "POST",
        body: JSON.stringify({ cart_id: cartId, customer_id: customerId, variant_id: variantId, quantity })
      });
    },
    async setCartQuantity(cartId, variantId, quantity) {
      return request("/api/cart/quantity", {
        method: "POST",
        body: JSON.stringify({ cart_id: cartId, variant_id: variantId, quantity })
      });
    },
    async removeFromCart(cartId, variantId) {
      return request("/api/cart/remove", {
        method: "POST",
        body: JSON.stringify({ cart_id: cartId, variant_id: variantId })
      });
    },

    // Checkout
    async previewCheckout(cartId, voucherCode) {
      return request("/api/checkout/preview", {
        method: "POST",
        body: JSON.stringify({ cart_id: cartId, voucher_code: voucherCode || "" })
      });
    },

    // Full checkout: syncs localStorage cart to server then places order
    async checkout({ method = "Cash", voucherCode } = {}) {
      const session = window.storefrontState ? window.storefrontState.getSession() : null;
      if (!session || !session.customerId) {
        return { ok: false, error: "Not signed in" };
      }
      const cartItems = window.storefrontState ? window.storefrontState.getCart() : [];
      if (!cartItems.length) {
        return { ok: false, error: "Cart is empty" };
      }
      const customer = window.storefrontState ? window.storefrontState.getCustomerProfile() : null;
      const address = customer?.address || {};
      const cartId = "cart-" + session.customerId + "-" + String(Date.now());
      // Sync each cart item to the server cart
      for (const item of cartItems) {
        const r = await this.addToCart(cartId, session.customerId, item.variantId, item.quantity);
        if (!r.ok && r.error !== "API unavailable") {
          return { ok: false, error: `Cart sync failed: ${r.error}` };
        }
      }
      const orderId = "order-" + session.customerId + "-" + String(Date.now());
      return request("/api/checkout", {
        method: "POST",
        body: JSON.stringify({
          order_id: orderId,
          cart_id: cartId,
          customer_id: session.customerId,
          method,
          voucher_code: voucherCode || "",
          recipient_name: address.recipientName || customer?.fullName || session.displayName || session.username,
          phone: address.phone || customer?.phone || "0900000000",
          line1: address.line1 || "12 Nguyen Hue",
          line2: address.line2 || "",
          ward: address.ward || "Ben Nghe",
          district: address.district || "District 1",
          city: address.city || customer?.city || "Ho Chi Minh City",
          country: address.country || "Vietnam"
        })
      });
    },

    // Orders
    async getOrder(orderId) {
      return request(`/api/orders/${orderId}`);
    },
    async getCustomerOrders(customerId) {
      return request(`/api/customers/${customerId}/orders`);
    },
    async payOrder(orderId) {
      return request(`/api/orders/${orderId}/pay`, { method: "POST" });
    },
    async advanceOrder(orderId, status) {
      return request(`/api/orders/${orderId}/advance`, {
        method: "POST",
        body: JSON.stringify({ status })
      });
    },
    async cancelOrder(orderId) {
      return request(`/api/orders/${orderId}/cancel`, { method: "POST" });
    },
    async getOrderReturns(orderId) {
      return request(`/api/orders/${orderId}/returns`);
    },

    // Reviews
    async createReview({ orderId, reviewId, customerId, productId, variantId, rating, comment }) {
      return request("/api/reviews", {
        method: "POST",
        body: JSON.stringify({
          order_id: orderId,
          review_id: reviewId,
          customer_id: customerId,
          product_id: productId,
          variant_id: variantId || "",
          rating,
          comment
        })
      });
    },
    async getCustomerReviews(customerId) {
      return request(`/api/customers/${customerId}/reviews`);
    },

    // Returns
    async createReturn({ orderId, returnId, orderItemId, quantity, reason }) {
      return request("/api/returns", {
        method: "POST",
        body: JSON.stringify({
          order_id: orderId,
          return_id: returnId,
          order_item_id: orderItemId,
          quantity,
          reason
        })
      });
    },
    async approveReturn(returnId) {
      return request(`/api/returns/${returnId}/approve`, { method: "POST" });
    },
    async rejectReturn(returnId) {
      return request(`/api/returns/${returnId}/reject`, { method: "POST" });
    },
    async restockReturn(returnId) {
      return request(`/api/returns/${returnId}/restock`, { method: "POST" });
    },
    async refundReturn(returnId) {
      return request(`/api/returns/${returnId}/refund`, { method: "POST" });
    },
    async closeReturn(returnId) {
      return request(`/api/returns/${returnId}/close`, { method: "POST" });
    },

    // Staff: catalog
    async createProduct({ productId, name, category, description, collection, status = "Active" }) {
      return request("/api/staff/products", {
        method: "POST",
        body: JSON.stringify({ product_id: productId, name, category, description, collection, status })
      });
    },
    async updateProductStatus(productId, status) {
      return request(`/api/staff/products/${productId}/status`, {
        method: "POST",
        body: JSON.stringify({ status })
      });
    },
    async addProductVariant(productId, { variantId, sku, size, color, priceMinor }) {
      return request(`/api/staff/products/${productId}/variants`, {
        method: "POST",
        body: JSON.stringify({ variant_id: variantId, sku, size, color, price_minor: priceMinor })
      });
    },
    async setInventory({ variantId, onHand, reserved = 0 }) {
      return request("/api/staff/inventory", {
        method: "POST",
        body: JSON.stringify({ variant_id: variantId, on_hand: onHand, reserved })
      });
    },
    async restockInventory(variantId, quantity) {
      return request(`/api/staff/inventory/${variantId}/restock`, {
        method: "POST",
        body: JSON.stringify({ quantity })
      });
    },
    async saveVoucher({ code, ratePercent, minOrderMinor, maxDiscountMinor, maxUses }) {
      return request("/api/staff/vouchers", {
        method: "POST",
        body: JSON.stringify({
          code,
          rate_percent: ratePercent,
          min_order_minor: minOrderMinor,
          max_discount_minor: maxDiscountMinor,
          max_uses: maxUses
        })
      });
    },

    // Staff: orders
    async getStaffOrders() {
      return request("/api/staff/orders");
    },
    async advanceStaffOrder(orderId, status) {
      return request(`/api/staff/orders/${orderId}/advance`, {
        method: "POST",
        body: JSON.stringify({ status })
      });
    },
    async cancelStaffOrder(orderId) {
      return request(`/api/staff/orders/${orderId}/cancel`, { method: "POST" });
    },

    // Payment
    async authorizePayment(orderId) {
      return request("/api/payment/authorize", {
        method: "POST",
        body: JSON.stringify({ order_id: orderId })
      });
    },

    // Shipping
    async getShippingQuote(method = "Standard") {
      return request(`/api/shipping/quote?method=${method}`);
    },
    async createShipment(orderId, method = "Standard") {
      return request("/api/shipping/shipments", {
        method: "POST",
        body: JSON.stringify({ order_id: orderId, method })
      });
    },

    // Reports
    async getRevenue() {
      return request("/api/reports/revenue");
    },
    async getBestSelling() {
      return request("/api/reports/best-selling");
    },
    async getLowStock(threshold = 5) {
      return request(`/api/reports/low-stock?threshold=${threshold}`);
    }
  };
})();
