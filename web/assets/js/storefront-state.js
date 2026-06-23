(function attachStorefrontState() {
  function createStorage() {
    try {
      const probeKey = "__store_manage_probe__";
      window.sessionStorage.setItem(probeKey, "1");
      window.sessionStorage.removeItem(probeKey);
      return window.sessionStorage;
    } catch {
      const memory = new Map();
      return {
        getItem(key) {
          return memory.has(key) ? memory.get(key) : null;
        },
        setItem(key, value) {
          memory.set(key, String(value));
        },
        removeItem(key) {
          memory.delete(key);
        }
      };
    }
  }

  const storage = createStorage();
  const storageKeys = {
    session: "fashion_store_session",
    cart: "fashion_store_cart",
    orders: "fashion_store_orders",
    products: "fashion_store_products",
    voucher: "fashion_store_voucher",
    accounts: "fashion_store_accounts",
    customers: "fashion_store_customers"
  };

  function readJson(key, fallback) {
    try {
      const raw = storage.getItem(key);
      return raw ? JSON.parse(raw) : fallback;
    } catch {
      return fallback;
    }
  }

  function writeJson(key, value) {
    storage.setItem(key, JSON.stringify(value));
  }

  function cloneData(value) {
    return JSON.parse(JSON.stringify(value));
  }

  function normalizeMoney(value, fallback = 0) {
    const numeric = Number(value);
    return Number.isFinite(numeric) && numeric >= 0 ? Math.round(numeric) : fallback;
  }

  function normalizeRate(value, fallback = 0) {
    const numeric = Number(value);
    if (!Number.isFinite(numeric)) {
      return fallback;
    }
    return Math.min(1, Math.max(0, numeric));
  }

  function normalizeProducts(products) {
    return (products || []).map((product) => ({
      ...product,
      status: ["Active", "Draft", "Archived"].includes(product.status) ? product.status : "Active",
      priceMinor: normalizeMoney(product.priceMinor, 0),
      variants: (product.variants || []).map((variant) => ({
        ...variant,
        stockQuantity: normalizeMoney(variant.stockQuantity, 8)
      }))
    }));
  }

  function normalizeVoucher(voucher) {
    return {
      code: String(voucher?.code || "WELCOME10").trim() || "WELCOME10",
      rate: normalizeRate(voucher?.rate, 0.1),
      maxDiscountMinor: normalizeMoney(voucher?.maxDiscountMinor, 800000),
      minOrderMinor: normalizeMoney(voucher?.minOrderMinor, 1000000),
      isActive: voucher?.isActive !== false
    };
  }

  function formatMoney(minor) {
    return new Intl.NumberFormat("vi-VN").format(minor) + " VND";
  }

  function buildDefaultAddress(fullName, phone, city = "Ho Chi Minh City", line1 = "12 Nguyen Hue") {
    return {
      recipientName: fullName || "Customer",
      phone: phone || "0900000000",
      line1: line1 || "12 Nguyen Hue",
      line2: "",
      ward: "Ben Nghe",
      district: "District 1",
      city: city || "Ho Chi Minh City",
      country: "Vietnam"
    };
  }

  function normalizeCustomerRecord(customer) {
    const fullName = String(customer?.fullName || customer?.full_name || "").trim();
    const phone = String(customer?.phone || "").trim();
    const city = String(customer?.city || customer?.address?.city || customer?.address?.City || "Ho Chi Minh City").trim() || "Ho Chi Minh City";
    const rawAddress = customer?.address || {};
    const address = {
      recipientName: String(rawAddress.recipientName || rawAddress.recipient_name || fullName || "Customer").trim(),
      phone: String(rawAddress.phone || phone || "0900000000").trim(),
      line1: String(rawAddress.line1 || "12 Nguyen Hue").trim(),
      line2: String(rawAddress.line2 || "").trim(),
      ward: String(rawAddress.ward || "Ben Nghe").trim(),
      district: String(rawAddress.district || "District 1").trim(),
      city,
      country: String(rawAddress.country || "Vietnam").trim()
    };
    return {
      customerId: String(customer?.customerId || customer?.customer_id || "").trim(),
      accountId: String(customer?.accountId || customer?.account_id || "").trim(),
      fullName,
      phone,
      city,
      address,
      wishlist: Array.isArray(customer?.wishlist)
        ? Array.from(new Set(customer.wishlist.map((entry) => String(entry).trim()).filter(Boolean)))
        : []
    };
  }

  function normalizeAccountRecord(account, fallback = {}) {
    return {
      accountId: String(account?.accountId || account?.account_id || fallback.accountId || "").trim(),
      username: String(account?.username || fallback.username || "").trim(),
      passwordHash: String(account?.passwordHash || account?.password_hash || fallback.passwordHash || "").trim(),
      role: account?.role ?? fallback.role ?? "Customer",
      customerId: account?.customerId ?? account?.customer_id ?? fallback.customerId ?? null,
      employeeId: account?.employeeId ?? account?.employee_id ?? fallback.employeeId ?? null
    };
  }

  function getAccounts() {
    const seed = cloneData(window.storefrontData.accounts || []);
    return (readJson(storageKeys.accounts, seed) || []).map((entry) => normalizeAccountRecord(entry));
  }

  function persistAccounts(accounts) {
    const normalized = (accounts || []).map((entry) => normalizeAccountRecord(entry));
    writeJson(storageKeys.accounts, normalized);
    return normalized;
  }

  function getCustomers() {
    const seed = cloneData(window.storefrontData.customers || []);
    return (readJson(storageKeys.customers, seed) || []).map((entry) => normalizeCustomerRecord(entry));
  }

  function persistCustomers(customers) {
    const normalized = (customers || []).map((entry) => normalizeCustomerRecord(entry));
    writeJson(storageKeys.customers, normalized);
    return normalized;
  }

  function upsertAccount(rawAccount) {
    const accounts = getAccounts();
    const index = accounts.findIndex((entry) =>
      entry.accountId === (rawAccount?.accountId || rawAccount?.account_id || "") ||
      entry.username === (rawAccount?.username || "")
    );
    const current = index >= 0 ? accounts[index] : {};
    const next = normalizeAccountRecord(rawAccount, current);
    if (index >= 0) {
      accounts[index] = next;
    } else {
      accounts.push(next);
    }
    persistAccounts(accounts);
    return next;
  }

  function upsertCustomer(rawCustomer) {
    const customers = getCustomers();
    const index = customers.findIndex((entry) =>
      entry.customerId === (rawCustomer?.customerId || rawCustomer?.customer_id || "") ||
      entry.accountId === (rawCustomer?.accountId || rawCustomer?.account_id || "")
    );
    const current = index >= 0 ? customers[index] : {};
    const next = normalizeCustomerRecord({ ...current, ...rawCustomer });
    if (index >= 0) {
      customers[index] = next;
    } else {
      customers.push(next);
    }
    persistCustomers(customers);
    return next;
  }

  function buildSession(account, customer) {
    const employee = (window.storefrontData.employees || []).find((entry) => entry.employeeId === account.employeeId);
    return {
      accountId: account.accountId,
      username: account.username,
      role: account.role,
      customerId: account.customerId,
      employeeId: account.employeeId,
      displayName: customer ? customer.fullName : (employee ? employee.fullName : account.username),
      customerName: customer ? customer.fullName : account.username
    };
  }

  function getSession() {
    return readJson(storageKeys.session, null);
  }

  function getCustomerProfile() {
    const session = getSession();
    if (!session || !session.customerId) {
      return null;
    }
    return getCustomers().find((entry) => entry.customerId === session.customerId) || null;
  }

  function normalizePasswordHash(password) {
    return password.startsWith("hash:") ? password : `hash:${password}`;
  }

  function slugify(value) {
    const slug = String(value || "")
      .toLowerCase()
      .replace(/[^a-z0-9]+/g, "-")
      .replace(/^-+|-+$/g, "");
    return slug || "customer";
  }

  function signIn(username, passwordHash) {
    const normalizedPasswordHash = normalizePasswordHash(passwordHash);
    const account = getAccounts().find(
      (entry) => entry.username === username && entry.passwordHash === normalizedPasswordHash
    );
    if (!account) {
      return { ok: false, error: "Invalid credentials for demo account." };
    }

    const customer = getCustomers().find(
      (entry) => entry.customerId === account.customerId
    );
    const session = buildSession(account, customer);
    writeJson(storageKeys.session, session);
    return { ok: true, session };
  }

  async function signInWithApi(username, passwordHash) {
    const normalizedPasswordHash = normalizePasswordHash(passwordHash);
    if (window.storefrontApi) {
      const response = await window.storefrontApi.signIn(username, normalizedPasswordHash);
      if (response.ok && response.data) {
        const account = upsertAccount({
          accountId: response.data.accountId || response.data.account_id,
          username: response.data.username,
          passwordHash: normalizedPasswordHash,
          role: response.data.role,
          customerId: response.data.customerId || response.data.customer_id
        });
        const customer = response.data.customer ? upsertCustomer(response.data.customer) : (
          account.customerId ? getCustomers().find((entry) => entry.customerId === account.customerId) || null : null
        );
        const session = buildSession(account, customer);
        if (response.data.displayName) {
          session.displayName = response.data.displayName;
        }
        writeJson(storageKeys.session, session);
        return { ok: true, session };
      }
    }
    return signIn(username, normalizedPasswordHash);
  }

  async function registerCustomer({ fullName, phone, city, line1, username, password }) {
    const cleanFullName = String(fullName || "").trim();
    const cleanPhone = String(phone || "").trim();
    const cleanCity = String(city || "").trim() || "Ho Chi Minh City";
    const cleanLine1 = String(line1 || "").trim() || "12 Nguyen Hue";
    const cleanUsername = String(username || "").trim();
    const cleanPassword = String(password || "").trim();
    if (!cleanFullName || !cleanPhone || !cleanUsername || !cleanPassword) {
      return { ok: false, error: "Full name, phone, username, and password are required." };
    }

    const normalizedPasswordHash = normalizePasswordHash(cleanPassword);
    const address = buildDefaultAddress(cleanFullName, cleanPhone, cleanCity, cleanLine1);

    if (window.storefrontApi) {
      const response = await window.storefrontApi.registerCustomer({
        username: cleanUsername,
        passwordHash: normalizedPasswordHash,
        fullName: cleanFullName,
        phone: cleanPhone,
        address
      });
      if (response.ok && response.data) {
        const account = upsertAccount({
          accountId: response.data.accountId || response.data.account_id,
          username: response.data.username,
          passwordHash: normalizedPasswordHash,
          role: response.data.role,
          customerId: response.data.customerId || response.data.customer_id
        });
        const customer = response.data.customer
          ? upsertCustomer(response.data.customer)
          : upsertCustomer({
              customerId: account.customerId,
              accountId: account.accountId,
              fullName: cleanFullName,
              phone: cleanPhone,
              city: cleanCity,
              address,
              wishlist: []
            });
        const session = buildSession(account, customer);
        writeJson(storageKeys.session, session);
        return { ok: true, session };
      }
      if (response.error && response.error !== "API unavailable") {
        return response;
      }
    }

    const accounts = getAccounts();
    if (accounts.some((entry) => entry.username.toLowerCase() === cleanUsername.toLowerCase())) {
      return { ok: false, error: "Username already exists in this local session." };
    }

    const slug = slugify(cleanUsername);
    const account = normalizeAccountRecord({
      accountId: `account-${slug}`,
      username: cleanUsername,
      passwordHash: normalizedPasswordHash,
      role: "Customer",
      customerId: `customer-${slug}`
    });
    const customer = normalizeCustomerRecord({
      customerId: account.customerId,
      accountId: account.accountId,
      fullName: cleanFullName,
      phone: cleanPhone,
      city: cleanCity,
      address,
      wishlist: []
    });
    persistAccounts([...accounts, account]);
    persistCustomers([...getCustomers(), customer]);
    const session = buildSession(account, customer);
    writeJson(storageKeys.session, session);
    return { ok: true, session };
  }

  function signOut() {
    storage.removeItem(storageKeys.session);
  }

  function getProducts() {
    const products = readJson(storageKeys.products, null);
    return products ? normalizeProducts(products) : normalizeProducts(cloneData(window.storefrontData.products || []));
  }

  function getProduct(productId) {
    return getProducts().find((entry) => entry.productId === productId) || null;
  }

  function persistProducts(products) {
    const normalized = normalizeProducts(products);
    writeJson(storageKeys.products, normalized);
    return normalized;
  }

  function getVoucher() {
    const voucher = readJson(storageKeys.voucher, null);
    return voucher ? normalizeVoucher(voucher) : normalizeVoucher(window.storefrontData.voucher);
  }

  function persistVoucher(voucher) {
    const normalized = normalizeVoucher(voucher);
    writeJson(storageKeys.voucher, normalized);
    return normalized;
  }

  function updateProduct(productId, patch) {
    const products = getProducts();
    const index = products.findIndex((entry) => entry.productId === productId);
    if (index < 0) {
      return { ok: false, error: "Product not found." };
    }

    const current = products[index];
    const next = {
      ...current,
      name: String(patch.name ?? current.name).trim(),
      category: String(patch.category ?? current.category).trim(),
      description: String(patch.description ?? current.description).trim(),
      priceMinor: normalizeMoney(patch.priceMinor, current.priceMinor),
      status: ["Active", "Draft", "Archived"].includes(String(patch.status))
        ? String(patch.status)
        : current.status
    };

    if (!next.name || !next.category || !next.description) {
      return { ok: false, error: "Product name, category, and description cannot be empty." };
    }

    products[index] = next;
    persistProducts(products);
    return { ok: true, product: next, products };
  }

  function updateVariantStock(productId, variantId, stockQuantity) {
    const products = getProducts();
    const product = products.find((entry) => entry.productId === productId);
    if (!product) {
      return { ok: false, error: "Product not found." };
    }

    const variant = product.variants.find((entry) => entry.variantId === variantId);
    if (!variant) {
      return { ok: false, error: "Variant not found." };
    }

    const normalizedStock = normalizeMoney(stockQuantity, -1);
    if (normalizedStock < 0) {
      return { ok: false, error: "Stock must be zero or greater." };
    }

    variant.stockQuantity = normalizedStock;
    persistProducts(products);
    return { ok: true, product, variant };
  }

  function updateVoucher(patch) {
    const current = getVoucher();
    const next = normalizeVoucher({
      ...current,
      ...patch,
      isActive: patch.isActive ?? current.isActive
    });

    if (!next.code) {
      return { ok: false, error: "Voucher code cannot be empty." };
    }

    persistVoucher(next);
    return { ok: true, voucher: next };
  }

  function getAdminReport() {
    const products = getProducts();
    const orders = getOrders();
    const voucher = getVoucher();
    const revenueMinor = orders.reduce((sum, order) => {
      if (order.paymentStatus === "Refunded") {
        return sum - order.totalMinor;
      }
      if (order.paymentStatus === "Paid") {
        return sum + order.totalMinor;
      }
      return sum;
    }, 0);

    const quantitiesByProduct = new Map();
    orders
      .filter((order) => order.orderStatus !== "Cancelled")
      .forEach((order) => {
        order.items.forEach((item) => {
          quantitiesByProduct.set(item.productName, (quantitiesByProduct.get(item.productName) || 0) + item.quantity);
        });
      });

    const bestSelling = Array.from(quantitiesByProduct.entries())
      .sort((left, right) => right[1] - left[1])[0] || null;

    const lowStockVariants = products.flatMap((product) => product.variants
      .filter((variant) => variant.stockQuantity <= 2)
      .map((variant) => ({
        productName: product.name,
        variantLabel: `${variant.color} / Size ${variant.size}`,
        stockQuantity: variant.stockQuantity
      })));

    return {
      totalOrders: orders.length,
      openOrders: orders.filter((entry) => !["Completed", "Cancelled"].includes(entry.orderStatus)).length,
      paidOrders: orders.filter((entry) => entry.paymentStatus === "Paid").length,
      refundedOrders: orders.filter((entry) => entry.paymentStatus === "Refunded").length,
      revenueMinor,
      activeProducts: products.filter((entry) => entry.status === "Active").length,
      voucher,
      bestSelling: bestSelling ? { productName: bestSelling[0], quantity: bestSelling[1] } : null,
      lowStockVariants,
      recentOrders: orders.slice(0, 5)
    };
  }

  function updateCustomerRecord(customerId, updater) {
    const customers = getCustomers();
    const index = customers.findIndex((entry) => entry.customerId === customerId);
    if (index < 0) {
      return { ok: false, error: "Customer profile not found." };
    }
    const current = { ...customers[index], address: { ...customers[index].address }, wishlist: [...customers[index].wishlist] };
    const updateResult = updater(current);
    if (updateResult && updateResult.ok === false) {
      return updateResult;
    }
    customers[index] = normalizeCustomerRecord(current);
    persistCustomers(customers);
    return { ok: true, customer: customers[index] };
  }

  function isInWishlist(productId) {
    const customer = getCustomerProfile();
    return Boolean(customer && customer.wishlist.includes(productId));
  }

  function getWishlistProducts() {
    const customer = getCustomerProfile();
    if (!customer) {
      return [];
    }
    const wishlist = new Set(customer.wishlist);
    return getProducts().filter((entry) => wishlist.has(entry.productId));
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
    if (product.status !== "Active") {
      return { ok: false, error: "Product is not active for sale." };
    }
    const variant = product.variants.find((entry) => entry.variantId === variantId);
    if (!variant) {
      return { ok: false, error: "Variant not found." };
    }

    const requestedQuantity = normalizeMoney(quantity, 0);
    if (requestedQuantity <= 0) {
      return { ok: false, error: "Quantity must be greater than zero." };
    }

    const existing = items.find((entry) => entry.variantId === variantId);
    const nextQuantity = (existing ? existing.quantity : 0) + requestedQuantity;
    if (nextQuantity > variant.stockQuantity) {
      return { ok: false, error: "Requested quantity exceeds current stock." };
    }

    if (existing) {
      existing.quantity = nextQuantity;
    } else {
      items.push({
        productId,
        variantId,
        quantity: requestedQuantity,
        unitPriceMinor: product.priceMinor
      });
    }

    persistCart(items);
    return { ok: true, items };
  }

  async function addToCartWithApi(productId, variantId, quantity) {
    if (window.storefrontApi) {
      const session = getSession();
      if (session && session.customerId) {
        const cartId = `cart-${session.customerId}`;
        const response = await window.storefrontApi.addToCart(cartId, session.customerId, variantId, quantity);
        if (response.ok || response.error === "API unavailable") {
          return addToCart(productId, variantId, quantity);
        }
        return response;
      }
      return addToCart(productId, variantId, quantity);
    }
    return addToCart(productId, variantId, quantity);
  }

  async function toggleWishlist(productId) {
    const session = getSession();
    if (!session || !session.customerId) {
      return { ok: false, error: "Sign in first to use wishlist." };
    }

    const saved = isInWishlist(productId);
    if (window.storefrontApi) {
      const response = saved
        ? await window.storefrontApi.removeFromWishlist(session.customerId, productId)
        : await window.storefrontApi.addToWishlist(session.customerId, productId);
      if (response.ok && response.data) {
        return {
          ok: true,
          saved: !saved,
          customer: upsertCustomer(response.data.customer || response.data)
        };
      }
      if (response.error && response.error !== "API unavailable") {
        return response;
      }
    }

    const local = updateCustomerRecord(session.customerId, (customer) => {
      const next = new Set(customer.wishlist);
      if (saved) {
        next.delete(productId);
      } else {
        next.add(productId);
      }
      customer.wishlist = Array.from(next);
    });
    if (!local.ok) {
      return local;
    }
    return { ok: true, saved: !saved, customer: local.customer };
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
    const product = getProduct(item.productId);
    const variant = product ? product.variants.find((entry) => entry.variantId === variantId) : null;
    if (!variant) {
      return { ok: false, error: "Variant not found." };
    }
    if (quantity > variant.stockQuantity) {
      return { ok: false, error: "Requested quantity exceeds current stock." };
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

  function getOrders() {
    return readJson(storageKeys.orders, []);
  }

  function getCustomerOrders() {
    const session = getSession();
    if (!session || !session.customerId) {
      return [];
    }
    return getOrders().filter((entry) => entry.customerId === session.customerId);
  }

  function persistOrders(orders) {
    writeJson(storageKeys.orders, orders);
    return orders;
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
    const voucher = getVoucher();
    let discountMinor = 0;
    if (voucher.isActive && subtotalMinor >= voucher.minOrderMinor) {
      discountMinor = Math.min(Math.round(subtotalMinor * voucher.rate), voucher.maxDiscountMinor);
    }
    return {
      items,
      subtotalMinor,
      discountMinor,
      totalMinor: subtotalMinor - discountMinor
    };
  }

  function normalizePaymentMethod(paymentMethod) {
    return ["Cash", "BankTransfer", "EWallet"].includes(paymentMethod) ? paymentMethod : "Cash";
  }

  function buildTrackingCode() {
    return `LOCAL-${Date.now().toString(36).toUpperCase()}`;
  }

  function buildOrderNumber(orders) {
    return `SM-${String(orders.length + 1).padStart(4, "0")}`;
  }

  function updateOrderRecord(orderId, updater) {
    const orders = getOrders();
    const index = orders.findIndex((entry) => entry.orderId === orderId);
    if (index < 0) {
      return { ok: false, error: "Order not found." };
    }

    const current = { ...orders[index] };
    const updateResult = updater(current);
    if (updateResult && updateResult.ok === false) {
      return updateResult;
    }

    current.updatedAtIso = new Date().toISOString();
    orders[index] = current;
    persistOrders(orders);
    return { ok: true, order: current, orders };
  }

  function applyInventoryDelta(items, delta) {
    const products = getProducts();
    for (const item of items) {
      const product = products.find((entry) => entry.productId === item.productId);
      const variant = product ? product.variants.find((entry) => entry.variantId === item.variantId) : null;
      if (!product || !variant) {
        return { ok: false, error: "Inventory record not found for one of the order items." };
      }
      if (delta < 0 && variant.stockQuantity < item.quantity) {
        return { ok: false, error: `Insufficient stock for ${item.productName}.` };
      }
    }

    for (const item of items) {
      const product = products.find((entry) => entry.productId === item.productId);
      const variant = product.variants.find((entry) => entry.variantId === item.variantId);
      variant.stockQuantity += delta * item.quantity;
    }

    persistProducts(products);
    return { ok: true, products };
  }

  function applyStaffOrderAction(orderId, action) {
    return updateOrderRecord(orderId, (order) => {
      switch (action) {
        case "mark-paid":
          if (order.orderStatus !== "AwaitingPayment") {
            return { ok: false, error: "Only awaiting-payment orders can be marked as paid." };
          }
          order.orderStatus = "Paid";
          order.paymentStatus = "Paid";
          order.shippingStatus = "Preparing";
          return { ok: true };
        case "pack":
          if (order.orderStatus !== "Paid") {
            return { ok: false, error: "Only paid orders can move to packed." };
          }
          order.orderStatus = "Packed";
          order.shippingStatus = "Preparing";
          return { ok: true };
        case "ship":
          if (order.orderStatus !== "Packed") {
            return { ok: false, error: "Only packed orders can move to shipped." };
          }
          order.orderStatus = "Shipped";
          order.shippingStatus = "InTransit";
          return { ok: true };
        case "deliver":
          if (order.orderStatus !== "Shipped") {
            return { ok: false, error: "Only shipped orders can move to delivered." };
          }
          order.orderStatus = "Delivered";
          order.shippingStatus = "Delivered";
          return { ok: true };
        case "complete":
          if (order.orderStatus !== "Delivered") {
            return { ok: false, error: "Only delivered orders can be completed." };
          }
          order.orderStatus = "Completed";
          order.shippingStatus = "Delivered";
          return { ok: true };
        case "cancel":
          if (!["AwaitingPayment", "Paid"].includes(order.orderStatus)) {
            return { ok: false, error: "Only awaiting-payment or paid orders can be cancelled in this demo." };
          }
          if (order.inventoryReserved && !order.inventoryRestored) {
            const inventoryResult = applyInventoryDelta(order.items, 1);
            if (!inventoryResult.ok) {
              return inventoryResult;
            }
            order.inventoryRestored = true;
          }
          order.orderStatus = "Cancelled";
          order.shippingStatus = "Failed";
          order.paymentStatus = order.paymentStatus === "Paid" ? "Refunded" : "Failed";
          return { ok: true };
        default:
          return { ok: false, error: "Unknown staff action." };
      }
    });
  }

  function createLocalOrderRecord(paymentMethod, summary, customer, session, backendState, paymentReference = null) {
    const normalizedPaymentMethod = normalizePaymentMethod(paymentMethod);
    const isInstantPaid = normalizedPaymentMethod === "EWallet";
    const existingOrders = getOrders();
    const createdAtIso = new Date().toISOString();
    return {
      orderId: `local-order-${Date.now()}`,
      orderNumber: buildOrderNumber(existingOrders),
      createdAtIso,
      customerId: session.customerId,
      customerName: customer ? customer.fullName : (session.displayName || session.customerName || session.username),
      paymentMethod: normalizedPaymentMethod,
      paymentStatus: isInstantPaid ? "Paid" : "Pending",
      orderStatus: isInstantPaid ? "Paid" : "AwaitingPayment",
      shippingMethod: "Standard",
      shippingStatus: "Preparing",
      trackingCode: buildTrackingCode(),
      backendState,
      paymentReference,
      voucherCode: summary.discountMinor > 0 ? getVoucher().code : null,
      subtotalMinor: summary.subtotalMinor,
      discountMinor: summary.discountMinor,
      totalMinor: summary.totalMinor,
      items: summary.items,
      inventoryReserved: true,
      inventoryRestored: false
    };
  }

  async function placeOrder(paymentMethod) {
    const session = getSession();
    if (!session || !session.customerId) {
      return { ok: false, error: "Sign in with a customer account to place a local order." };
    }

    const summary = buildCartSummary();
    if (summary.items.length === 0) {
      return { ok: false, error: "Your cart is empty." };
    }

    const inventoryResult = applyInventoryDelta(summary.items, -1);
    if (!inventoryResult.ok) {
      return inventoryResult;
    }

    const customer = getCustomerProfile();
    const normalizedPaymentMethod = normalizePaymentMethod(paymentMethod);
    let backendState = "Local demo";
    let backendMessage = "Saved locally without live backend.";
    let paymentReference = null;

    if (window.storefrontApi) {
      try {
        const response = await window.storefrontApi.checkout({ method: normalizedPaymentMethod });
        if (response.ok) {
          backendState = "API connected";
          backendMessage = "Submitted to backend checkout endpoint.";
          const backendOrderId = response.data?.order?.order_id || response.data?.order?.orderId || null;
          if (backendOrderId && normalizedPaymentMethod === "EWallet") {
            const paymentResult = await window.storefrontApi.authorizePayment(backendOrderId);
            if (paymentResult.ok) {
              paymentReference = paymentResult.data?.reference || null;
              const markPaidResult = await window.storefrontApi.payOrder(backendOrderId);
              backendMessage = markPaidResult.ok
                ? (paymentReference
                  ? `Backend order paid via E-Wallet (${paymentReference}).`
                  : "Backend order paid via E-Wallet.")
                : `Backend payment authorized${paymentReference ? ` (${paymentReference})` : ""} but mark-paid failed: ${markPaidResult.error}`;
            } else {
              backendMessage = `Backend order created but e-wallet authorization failed: ${paymentResult.error}`;
            }
          } else if (backendOrderId && normalizedPaymentMethod === "BankTransfer") {
            const paymentResult = await window.storefrontApi.authorizePayment(backendOrderId);
            if (paymentResult.ok) {
              paymentReference = paymentResult.data?.reference || null;
              backendMessage = paymentReference
                ? `Backend order is awaiting bank transfer. Reference: ${paymentReference}.`
                : "Backend order is awaiting bank transfer confirmation.";
            } else {
              backendMessage = `Backend order created but transfer reference failed: ${paymentResult.error}`;
            }
          }
        } else if (response.error) {
          backendMessage = `Backend checkout unavailable: ${response.error}. Kept local demo order.`;
        }
      } catch (error) {
        backendMessage = `Backend checkout threw an error: ${error instanceof Error ? error.message : "Unknown error"}. Kept local demo order.`;
      }
    }

    const orders = getOrders();
    const order = createLocalOrderRecord(
      normalizedPaymentMethod,
      summary,
      customer,
      session,
      backendState,
      paymentReference
    );
    persistOrders([order, ...orders]);
    clearCart();

    return {
      ok: true,
      order,
      backendMessage
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
    getProducts,
    getVoucher,
    updateProduct,
    updateVariantStock,
    updateVoucher,
    getAdminReport,
    isInWishlist,
    getWishlistProducts,
    addToCart,
    addToCartWithApi,
    toggleWishlist,
    getCart,
    setCartQuantity,
    removeFromCart,
    clearCart,
    getOrders,
    getCustomerOrders,
    buildCartSummary,
    placeOrder,
    applyStaffOrderAction,
    registerCustomer
  };
})();
