(function attachStorefrontState() {
  const storageKeys = {
    session: "fashion_store_session",
    cart: "fashion_store_cart"
  };

  function readJson(key, fallback) {
    try {
      const raw = window.localStorage.getItem(key);
      return raw ? JSON.parse(raw) : fallback;
    } catch {
      return fallback;
    }
  }

  function writeJson(key, value) {
    window.localStorage.setItem(key, JSON.stringify(value));
  }

  function formatMoney(minor) {
    return new Intl.NumberFormat("vi-VN").format(minor) + " VND";
  }

  function getSession() {
    return readJson(storageKeys.session, null);
  }

  function signIn(username, passwordHash) {
    const account = window.storefrontData.accounts.find(
      (entry) => entry.username === username && entry.passwordHash === passwordHash
    );
    if (!account) {
      return { ok: false, error: "Invalid credentials for demo account." };
    }

    const customer = window.storefrontData.customers.find(
      (entry) => entry.customerId === account.customerId
    );
    const session = {
      accountId: account.accountId,
      username: account.username,
      role: account.role,
      customerId: account.customerId,
      customerName: customer ? customer.fullName : account.username
    };
    writeJson(storageKeys.session, session);
    return { ok: true, session };
  }

  function signOut() {
    window.localStorage.removeItem(storageKeys.session);
  }

  function getProduct(productId) {
    return window.storefrontData.products.find((entry) => entry.productId === productId) || null;
  }

  function getCart() {
    return readJson(storageKeys.cart, []);
  }

  function persistCart(items) {
    writeJson(storageKeys.cart, items);
    return items;
  }

  function addToCart(productId, variantId, quantity) {
    const items = getCart();
    const product = getProduct(productId);
    if (!product) {
      return { ok: false, error: "Product not found." };
    }
    const variant = product.variants.find((entry) => entry.variantId === variantId);
    if (!variant) {
      return { ok: false, error: "Variant not found." };
    }

    const existing = items.find((entry) => entry.variantId === variantId);
    if (existing) {
      existing.quantity += quantity;
    } else {
      items.push({
        productId,
        variantId,
        quantity,
        unitPriceMinor: product.priceMinor
      });
    }

    persistCart(items);
    return { ok: true, items };
  }

  function setCartQuantity(variantId, quantity) {
    const items = getCart();
    const item = items.find((entry) => entry.variantId === variantId);
    if (!item) {
      return { ok: false, error: "Cart item not found." };
    }
    if (quantity <= 0) {
      return removeFromCart(variantId);
    }
    item.quantity = quantity;
    persistCart(items);
    return { ok: true, items };
  }

  function removeFromCart(variantId) {
    const items = getCart().filter((entry) => entry.variantId !== variantId);
    persistCart(items);
    return { ok: true, items };
  }

  function hydrateCartItems() {
    return getCart().map((entry) => {
      const product = getProduct(entry.productId);
      const variant = product ? product.variants.find((candidate) => candidate.variantId === entry.variantId) : null;
      return {
        ...entry,
        productName: product ? product.name : "Unknown Product",
        image: product ? product.images[0] : "",
        color: variant ? variant.color : "-",
        size: variant ? variant.size : "-",
        lineTotalMinor: entry.unitPriceMinor * entry.quantity
      };
    });
  }

  function buildCartSummary() {
    const items = hydrateCartItems();
    const subtotalMinor = items.reduce((sum, item) => sum + item.lineTotalMinor, 0);
    const voucher = window.storefrontData.voucher;
    let discountMinor = 0;
    if (subtotalMinor >= voucher.minOrderMinor) {
      discountMinor = Math.min(Math.round(subtotalMinor * voucher.rate), voucher.maxDiscountMinor);
    }
    return {
      items,
      subtotalMinor,
      discountMinor,
      totalMinor: subtotalMinor - discountMinor
    };
  }

  window.storefrontState = {
    formatMoney,
    getSession,
    signIn,
    signOut,
    getProduct,
    addToCart,
    setCartQuantity,
    removeFromCart,
    buildCartSummary
  };
})();
