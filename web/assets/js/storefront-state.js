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

  function getCustomerProfile() {
    const session = getSession();
    if (!session || !session.customerId) {
      return null;
    }
    return window.storefrontData.customers.find((entry) => entry.customerId === session.customerId) || null;
  }

  function normalizePasswordHash(password) {
    return password.startsWith("hash:") ? password : `hash:${password}`;
  }

  function signIn(username, passwordHash) {
    const normalizedPasswordHash = normalizePasswordHash(passwordHash);
    const account = window.storefrontData.accounts.find(
      (entry) => entry.username === username && entry.passwordHash === normalizedPasswordHash
    );
    if (!account) {
      return { ok: false, error: "Invalid credentials for demo account." };
    }

    const customer = window.storefrontData.customers.find(
      (entry) => entry.customerId === account.customerId
    );
    const employee = window.storefrontData.employees.find(
      (entry) => entry.employeeId === account.employeeId
    );
    const session = {
      accountId: account.accountId,
      username: account.username,
      role: account.role,
      customerId: account.customerId,
      employeeId: account.employeeId,
      displayName: customer ? customer.fullName : (employee ? employee.fullName : account.username),
      customerName: customer ? customer.fullName : account.username
    };
    writeJson(storageKeys.session, session);
    return { ok: true, session };
  }

  async function signInWithApi(username, passwordHash) {
    const normalizedPasswordHash = normalizePasswordHash(passwordHash);
    if (window.storefrontApi) {
      const response = await window.storefrontApi.signIn(username, normalizedPasswordHash);
      if (response.ok && response.data) {
        const account = response.data;
        const customer = window.storefrontData.customers.find(
          (entry) => entry.customerId === account.customerId
        );
        const session = {
          accountId: account.accountId || account.account_id,
          username: account.username,
          role: account.role,
          customerId: account.customerId || (customer ? customer.customerId : null),
          customerName: account.customerName || (customer ? customer.fullName : account.username)
        };
        writeJson(storageKeys.session, session);
        return { ok: true, session };
      }
    }
    return signIn(username, normalizedPasswordHash);
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

  async function addToCartWithApi(productId, variantId, quantity) {
    if (window.storefrontApi) {
      const response = await window.storefrontApi.addToCart(productId, variantId, quantity);
      if (response.ok) {
        return addToCart(productId, variantId, quantity);
      }
    }
    return addToCart(productId, variantId, quantity);
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

  function clearCart() {
    persistCart([]);
    return { ok: true, items: [] };
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
    if (voucher && subtotalMinor >= voucher.minOrderMinor) {
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
    getCustomerProfile,
    signIn,
    signInWithApi,
    signOut,
    getProduct,
    addToCart,
    addToCartWithApi,
    setCartQuantity,
    removeFromCart,
    clearCart,
    buildCartSummary
  };
})();
