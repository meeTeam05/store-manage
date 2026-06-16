(function attachStorefrontApi() {
  const baseUrl = window.STOREFRONT_API_BASE_URL || "";

  async function request(path, options = {}) {
    try {
      const response = await fetch(`${baseUrl}${path}`, {
        headers: {
          "Content-Type": "application/json",
          ...(options.headers || {})
        },
        ...options
      });
      if (!response.ok) {
        return { ok: false, error: `API request failed: ${response.status}` };
      }
      return { ok: true, data: await response.json() };
    } catch {
      return { ok: false, error: "API unavailable" };
    }
  }

  window.storefrontApi = {
    async signIn(username, passwordHash) {
      return request("/api/sign-in", {
        method: "POST",
        body: JSON.stringify({ username, passwordHash })
      });
    },

    async addToCart(productId, variantId, quantity) {
      const session = window.storefrontState ? window.storefrontState.getSession() : null;
      return request("/api/cart/items", {
        method: "POST",
        body: JSON.stringify({
          productId,
          variantId,
          quantity,
          customerId: session ? session.customerId : null
        })
      });
    },

    async checkout() {
      const session = window.storefrontState ? window.storefrontState.getSession() : null;
      const summary = window.storefrontState ? window.storefrontState.buildCartSummary() : null;
      return request("/api/checkout", {
        method: "POST",
        body: JSON.stringify({ session, summary })
      });
    }
  };
})();
