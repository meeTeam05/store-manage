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
  const productImageFallbacks = {
    Outerwear: [
      "https://images.unsplash.com/photo-1529139574466-a303027c1d8b?auto=format&fit=crop&w=1200&q=80",
      "https://images.unsplash.com/photo-1483985988355-763728e1935b?auto=format&fit=crop&w=1200&q=80"
    ],
    Dresses: [
      "https://images.unsplash.com/photo-1515886657613-9f3515b0c78f?auto=format&fit=crop&w=1200&q=80",
      "https://images.unsplash.com/photo-1524504388940-b1c1722653e1?auto=format&fit=crop&w=1200&q=80"
    ],
    Bottoms: [
      "https://images.unsplash.com/photo-1475180098004-ca77a66827be?auto=format&fit=crop&w=1200&q=80",
      "https://images.unsplash.com/photo-1506629905607-d9c297d2b2a7?auto=format&fit=crop&w=1200&q=80"
    ],
    Tops: [
      "https://images.unsplash.com/photo-1525507119028-ed4c629a60a3?auto=format&fit=crop&w=1200&q=80",
      "https://images.unsplash.com/photo-1489987707025-afc232f7ea0f?auto=format&fit=crop&w=1200&q=80"
    ],
    Shoes: [
      "https://images.unsplash.com/photo-1543163521-1bf539c55dd2?auto=format&fit=crop&w=1200&q=80",
      "https://images.unsplash.com/photo-1549298916-b41d501d3772?auto=format&fit=crop&w=1200&q=80"
    ],
    Accessories: [
      "https://images.unsplash.com/photo-1523205771623-e0faa4d2813d?auto=format&fit=crop&w=1200&q=80",
      "https://images.unsplash.com/photo-1509695507497-903c140c43b0?auto=format&fit=crop&w=1200&q=80"
    ]
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

  function fallbackImagesForProduct(product) {
    const local = (window.storefrontData.products || []).find((entry) => entry.productId === product.productId);
    if (local && Array.isArray(local.images) && local.images.length > 0) {
      return cloneData(local.images);
    }
    return cloneData(productImageFallbacks[product.category] || productImageFallbacks.Accessories);
  }

  function normalizeProductStatus(status) {
    return ["Active", "Draft", "Archived", "Discontinued"].includes(status) ? status : "Active";
  }

  function lowestVariantPrice(variants) {
    return variants.reduce((lowest, variant) => {
      const price = normalizeMoney(variant.priceMinor, 0);
      if (lowest === null || (price > 0 && price < lowest)) {
        return price;
      }
      return lowest;
    }, null) || 0;
  }

  function normalizeProducts(products) {
    return (products || []).map((product) => {
      const variants = (product.variants || []).map((variant) => ({
        ...variant,
        variantId: variant.variantId || variant.variant_id || "",
        sku: variant.sku || "",
        size: String(variant.size || ""),
        color: String(variant.color || ""),
        priceMinor: normalizeMoney(variant.priceMinor ?? variant.price_minor, 0),
        active: variant.active !== false,
        stockQuantity: normalizeMoney(variant.stockQuantity ?? variant.stock_quantity, 8)
      }));
      const normalized = {
        ...product,
        productId: product.productId || product.product_id || "",
        name: String(product.name || "Untitled Product"),
        category: String(product.category || "Accessories"),
        description: String(product.description || ""),
        collection: String(product.collection || ""),
        status: normalizeProductStatus(product.status),
        variants
      };
      normalized.priceMinor = normalizeMoney(product.priceMinor ?? product.price_minor, lowestVariantPrice(variants));
      normalized.variants = variants.map((variant) => ({
        ...variant,
        priceMinor: variant.priceMinor || normalized.priceMinor
      }));
      normalized.images = Array.isArray(product.images) && product.images.length > 0
        ? cloneData(product.images)
        : fallbackImagesForProduct(normalized);
      return normalized;
    });
  }

  function normalizeApiProducts(products) {
    const existingProducts = readJson(storageKeys.products, []);
    const existingStockByVariant = new Map();
    existingProducts.forEach((product) => {
      (product.variants || []).forEach((variant) => {
        const variantId = variant.variantId || variant.variant_id;
        if (variantId) {
          existingStockByVariant.set(variantId, variant.stockQuantity ?? variant.stock_quantity);
        }
      });
    });
    return normalizeProducts((products || []).map((product) => ({
      productId: product.product_id,
      name: product.name,
      category: product.category,
      description: product.description,
      collection: product.collection,
      status: product.status,
      variants: (product.variants || []).map((variant) => ({
        variantId: variant.variant_id,
        sku: variant.sku,
        size: variant.size,
        color: variant.color,
        priceMinor: variant.price_minor,
        active: variant.active,
        stockQuantity: normalizeMoney(
          variant.stock_quantity ?? variant.stockQuantity,
          existingStockByVariant.get(variant.variant_id) ?? 8
        )
      }))
    })));
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

  async function loadProductsWithApi(params = {}) {
    if (!window.storefrontApi) {
      return { ok: false, error: "API unavailable", products: getProducts(), source: "local" };
    }
    const response = await window.storefrontApi.getProducts(params);
    if (!response.ok || !Array.isArray(response.data)) {
      return {
        ok: false,
        error: response.error || "Catalog unavailable",
        products: getProducts(),
        source: response.error === "API unavailable" ? "local" : "error"
      };
    }
    const products = persistProducts(normalizeApiProducts(response.data));
    return { ok: true, products, source: "api" };
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

  function findProductByVariantId(products, variantId) {
    return products.find((product) => product.variants.some((variant) => variant.variantId === variantId)) || null;
  }

  function buildLocalId(prefix) {
    return `${prefix}-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 7)}`;
  }

  function createLocalStaffProduct(draft) {
    const products = getProducts();
    const productId = String(draft.productId || buildLocalId("product")).trim();
    if (products.some((product) => product.productId === productId)) {
      return { ok: false, error: "Product ID already exists." };
    }
    const product = normalizeProducts([{
      productId,
      name: String(draft.name || "").trim(),
      category: String(draft.category || "Accessories").trim(),
      description: String(draft.description || "").trim(),
      collection: String(draft.collection || "").trim(),
      status: String(draft.status || "Draft"),
      variants: []
    }])[0];
    if (!product.name || !product.category || !product.description) {
      return { ok: false, error: "Product name, category, and description cannot be empty." };
    }
    const nextProducts = persistProducts([product, ...products]);
    return { ok: true, product, products: nextProducts, source: "local" };
  }

  async function createStaffProductWithApi(draft) {
    const normalizedDraft = {
      productId: String(draft.productId || buildLocalId("product")).trim(),
      name: String(draft.name || "").trim(),
      category: String(draft.category || "Accessories").trim(),
      description: String(draft.description || "").trim(),
      collection: String(draft.collection || "").trim(),
      status: String(draft.status || "Draft")
    };
    if (!normalizedDraft.name || !normalizedDraft.category || !normalizedDraft.description) {
      return { ok: false, error: "Product name, category, and description cannot be empty." };
    }
    if (!window.storefrontApi) {
      return createLocalStaffProduct(normalizedDraft);
    }
    const response = await window.storefrontApi.createProduct(normalizedDraft);
    if (!response.ok) {
      return response.error === "API unavailable"
        ? createLocalStaffProduct(normalizedDraft)
        : { ok: false, error: `Backend product create failed: ${response.error}` };
    }
    const product = normalizeApiProducts([response.data])[0];
    const products = getProducts();
    const nextProducts = persistProducts([product, ...products.filter((entry) => entry.productId !== product.productId)]);
    return { ok: true, product, products: nextProducts, source: "api" };
  }

  function addLocalProductVariant(productId, draft) {
    const products = getProducts();
    const product = products.find((entry) => entry.productId === productId);
    if (!product) return { ok: false, error: "Product not found." };
    const variantId = String(draft.variantId || buildLocalId("variant")).trim();
    if (product.variants.some((variant) => variant.variantId === variantId)) {
      return { ok: false, error: "Variant ID already exists for this product." };
    }
    const variant = {
      variantId,
      sku: String(draft.sku || "").trim(),
      size: String(draft.size || "").trim(),
      color: String(draft.color || "").trim(),
      priceMinor: normalizeMoney(draft.priceMinor, product.priceMinor),
      active: true,
      stockQuantity: normalizeMoney(draft.stockQuantity, 0)
    };
    if (!variant.sku || !variant.size || !variant.color || variant.priceMinor <= 0) {
      return { ok: false, error: "Variant sku, size, color, and price are required." };
    }
    product.variants.push(variant);
    product.priceMinor = lowestVariantPrice(product.variants);
    persistProducts(products);
    return { ok: true, product, variant, products, source: "local" };
  }

  async function setStaffInventoryWithApi({ productId, variantId, onHand, reserved = 0 }) {
    const normalizedOnHand = normalizeMoney(onHand, -1);
    const normalizedReserved = normalizeMoney(reserved, 0);
    if (normalizedOnHand < 0 || normalizedReserved < 0 || normalizedReserved > normalizedOnHand) {
      return { ok: false, error: "Inventory values are invalid." };
    }
    if (!window.storefrontApi) {
      return updateVariantStock(productId, variantId, normalizedOnHand - normalizedReserved);
    }
    const response = await window.storefrontApi.setInventory({ variantId, onHand: normalizedOnHand, reserved: normalizedReserved });
    if (!response.ok) {
      return response.error === "API unavailable"
        ? updateVariantStock(productId, variantId, normalizedOnHand - normalizedReserved)
        : { ok: false, error: `Backend inventory set failed: ${response.error}` };
    }
    const products = getProducts();
    const product = productId ? products.find((entry) => entry.productId === productId) : findProductByVariantId(products, variantId);
    const variant = product ? product.variants.find((entry) => entry.variantId === variantId) : null;
    if (variant) {
      variant.stockQuantity = normalizeMoney(response.data.available ?? response.data.on_hand, normalizedOnHand - normalizedReserved);
      persistProducts(products);
    }
    return { ok: true, inventory: response.data, product, variant, source: "api" };
  }

  async function addStaffProductVariantWithApi(productId, draft) {
    const product = getProduct(productId);
    if (!product) return { ok: false, error: "Product not found." };
    const normalizedDraft = {
      variantId: String(draft.variantId || buildLocalId("variant")).trim(),
      sku: String(draft.sku || "").trim(),
      size: String(draft.size || "").trim(),
      color: String(draft.color || "").trim(),
      priceMinor: normalizeMoney(draft.priceMinor, product.priceMinor),
      stockQuantity: normalizeMoney(draft.stockQuantity, 0)
    };
    if (!normalizedDraft.sku || !normalizedDraft.size || !normalizedDraft.color || normalizedDraft.priceMinor <= 0) {
      return { ok: false, error: "Variant sku, size, color, and price are required." };
    }
    if (!window.storefrontApi) return addLocalProductVariant(productId, normalizedDraft);
    const response = await window.storefrontApi.addProductVariant(productId, normalizedDraft);
    if (!response.ok) {
      return response.error === "API unavailable"
        ? addLocalProductVariant(productId, normalizedDraft)
        : { ok: false, error: `Backend variant create failed: ${response.error}` };
    }
    const backendProduct = normalizeApiProducts([response.data])[0];
    const products = getProducts();
    persistProducts(products.map((entry) => entry.productId === backendProduct.productId ? backendProduct : entry));
    const inventoryResult = await setStaffInventoryWithApi({
      productId,
      variantId: normalizedDraft.variantId,
      onHand: normalizedDraft.stockQuantity,
      reserved: 0
    });
    if (!inventoryResult.ok) return inventoryResult;
    const updatedProduct = getProduct(productId) || backendProduct;
    const variant = updatedProduct.variants.find((entry) => entry.variantId === normalizedDraft.variantId) || null;
    return { ok: true, product: updatedProduct, variant, products: getProducts(), source: "api" };
  }

  async function restockStaffInventoryWithApi({ productId, variantId, quantity }) {
    const normalizedQuantity = normalizeMoney(quantity, -1);
    if (normalizedQuantity <= 0) {
      return { ok: false, error: "Restock quantity must be greater than zero." };
    }
    const localRestock = () => {
      const product = getProduct(productId);
      const variant = product ? product.variants.find((entry) => entry.variantId === variantId) : null;
      return product && variant
        ? updateVariantStock(product.productId, variantId, variant.stockQuantity + normalizedQuantity)
        : { ok: false, error: "Variant not found." };
    };
    if (!window.storefrontApi) {
      return localRestock();
    }
    const response = await window.storefrontApi.restockInventory(variantId, normalizedQuantity);
    if (!response.ok) {
      return response.error === "API unavailable"
        ? localRestock()
        : { ok: false, error: `Backend restock failed: ${response.error}` };
    }
    const products = getProducts();
    const product = productId
      ? products.find((entry) => entry.productId === productId)
      : findProductByVariantId(products, variantId);
    const variant = product ? product.variants.find((entry) => entry.variantId === variantId) : null;
    if (variant) {
      variant.stockQuantity = normalizeMoney(
        response.data.available ?? response.data.on_hand,
        variant.stockQuantity + normalizedQuantity
      );
      persistProducts(products);
    }
    return { ok: true, inventory: response.data, product, variant, source: "api" };
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
        unitPriceMinor: variant.priceMinor || product.priceMinor
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

  function persistOrderRecord(order) {
    const orders = getOrders();
    return persistOrders([order, ...orders.filter((entry) => entry.orderId !== order.orderId)]);
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

  function buildBackendTrackingCode(orderId) {
    const compact = String(orderId || Date.now()).replace(/[^a-z0-9]/gi, "").slice(-10).toUpperCase();
    return `API-${compact || Date.now().toString(36).toUpperCase()}`;
  }

  function buildOrderNumber(orders) {
    return `SM-${String(orders.length + 1).padStart(4, "0")}`;
  }

  function buildBackendOrderNumber(orderId) {
    const compact = String(orderId || "").replace(/^order-/i, "").replace(/[^a-z0-9]/gi, "-");
    return compact ? `API-${compact.toUpperCase()}` : `API-${Date.now().toString(36).toUpperCase()}`;
  }

  function normalizeOrderStatus(status) {
    return ["Draft", "AwaitingPayment", "Paid", "Packed", "Shipped", "Delivered", "Completed", "Cancelled", "Returned"].includes(status)
      ? status
      : "AwaitingPayment";
  }

  function normalizePaymentStatus(status) {
    return ["Unpaid", "Pending", "Paid", "Failed", "Refunded"].includes(status) ? status : "Pending";
  }

  function shippingStatusFromOrderStatus(orderStatus) {
    if (orderStatus === "Shipped") return "InTransit";
    if (["Delivered", "Completed"].includes(orderStatus)) return "Delivered";
    if (orderStatus === "Cancelled") return "Failed";
    if (orderStatus === "Returned") return "Returned";
    return "Preparing";
  }

  function normalizeBackendOrder(responseData, summary = {}, customer = null, session = {}, paymentReference = null) {
    const payload = responseData?.order || responseData;
    if (!payload || !(payload.order_id || payload.orderId)) return null;
    const orderId = payload.order_id || payload.orderId;
    const orderStatus = normalizeOrderStatus(payload.status || payload.orderStatus);
    const items = (payload.items || []).map((item) => {
      const unitPriceMinor = normalizeMoney(item.unit_price_minor ?? item.unitPriceMinor, 0);
      const quantity = normalizeMoney(item.quantity, 1);
      return {
        orderItemId: item.order_item_id || item.orderItemId || `${orderId}-${item.variant_id || item.variantId}`,
        productId: item.product_id || item.productId || "",
        variantId: item.variant_id || item.variantId || "",
        productName: item.product_name || item.productName || "Unknown Product",
        sku: item.sku || "",
        size: item.size || "-",
        color: item.color || "-",
        unitPriceMinor,
        quantity,
        lineTotalMinor: normalizeMoney(item.line_total_minor ?? item.lineTotalMinor, unitPriceMinor * quantity)
      };
    });
    return {
      orderId,
      orderNumber: payload.order_number || payload.orderNumber || buildBackendOrderNumber(orderId),
      createdAtIso: payload.created_at || payload.createdAtIso || new Date().toISOString(),
      updatedAtIso: payload.updated_at || payload.updatedAtIso || new Date().toISOString(),
      customerId: payload.customer_id || payload.customerId || session.customerId || "",
      customerName: payload.customer_name || payload.customerName || customer?.fullName || session.displayName || session.username || "Unknown Customer",
      paymentMethod: normalizePaymentMethod(payload.payment_method || payload.paymentMethod),
      paymentStatus: normalizePaymentStatus(payload.payment_status || payload.paymentStatus),
      orderStatus,
      shippingMethod: payload.shipping_method || payload.shippingMethod || "Standard",
      shippingStatus: payload.shipping_status || payload.shippingStatus || shippingStatusFromOrderStatus(orderStatus),
      trackingCode: payload.tracking_code || payload.trackingCode || buildBackendTrackingCode(orderId),
      backendState: "API connected",
      paymentReference: paymentReference || payload.payment_reference || payload.paymentReference || null,
      voucherCode: payload.voucher_code || payload.voucherCode || null,
      subtotalMinor: normalizeMoney(responseData?.subtotal_minor ?? payload.subtotal_minor ?? payload.subtotalMinor, summary.subtotalMinor || 0),
      discountMinor: normalizeMoney(responseData?.discount_minor ?? payload.discount_minor ?? payload.discountMinor, summary.discountMinor || 0),
      totalMinor: normalizeMoney(responseData?.total_minor ?? payload.total_minor ?? payload.totalMinor, summary.totalMinor || 0),
      items: items.length > 0 ? items : (summary.items || []),
      inventoryReserved: true,
      inventoryRestored: false,
      source: "backend"
    };
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

  function mergeBackendOrders(backendOrders) {
    const existingOrders = getOrders();
    const backendIds = new Set(backendOrders.map((order) => order.orderId));
    return persistOrders([
      ...backendOrders,
      ...existingOrders.filter((order) => !backendIds.has(order.orderId) && order.source !== "backend")
    ]);
  }

  async function loadStaffOrdersWithApi() {
    if (!window.storefrontApi) {
      return { ok: false, error: "API unavailable", orders: getOrders(), source: "local" };
    }
    const response = await window.storefrontApi.getStaffOrders();
    if (!response.ok || !Array.isArray(response.data)) {
      return {
        ok: false,
        error: response.error || "Staff orders unavailable",
        orders: getOrders(),
        source: response.error === "API unavailable" ? "local" : "error"
      };
    }
    const orders = mergeBackendOrders(response.data.map((order) => normalizeBackendOrder(order)).filter(Boolean));
    return { ok: true, orders, source: "api" };
  }

  function staffActionTargetStatus(action) {
    return { pack: "Packed", ship: "Shipped", deliver: "Delivered", complete: "Completed" }[action] || "";
  }

  async function applyStaffOrderActionWithApi(orderId, action) {
    const existingOrder = getOrders().find((order) => order.orderId === orderId) || null;
    const isBackendOrder = existingOrder && (existingOrder.source === "backend" || existingOrder.backendState === "API connected");
    if (!window.storefrontApi || !isBackendOrder) {
      return applyStaffOrderAction(orderId, action);
    }
    let response;
    if (action === "mark-paid") {
      response = await window.storefrontApi.payOrder(orderId);
    } else if (action === "cancel") {
      response = await window.storefrontApi.cancelStaffOrder(orderId);
    } else {
      const status = staffActionTargetStatus(action);
      if (!status) return { ok: false, error: "Unknown staff action." };
      response = await window.storefrontApi.advanceStaffOrder(orderId, status);
    }
    if (!response.ok) {
      return response.error === "API unavailable"
        ? applyStaffOrderAction(orderId, action)
        : { ok: false, error: `Backend staff action failed: ${response.error}` };
    }
    const customer = existingOrder ? { fullName: existingOrder.customerName } : null;
    const order = normalizeBackendOrder(
      response.data,
      existingOrder || {},
      customer,
      getSession() || {},
      existingOrder?.paymentReference || null
    );
    if (!order) return { ok: false, error: "Backend staff action did not return an order." };
    persistOrderRecord(order);
    return { ok: true, order, orders: getOrders(), source: "api" };
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
    let backendMessage = "Saved locally without live backend.";
    let paymentReference = null;
    let order = null;

    if (window.storefrontApi) {
      try {
        const response = await window.storefrontApi.checkout({ method: normalizedPaymentMethod });
        if (response.ok) {
          const backendOrderId = response.data?.order?.order_id || response.data?.order?.orderId || null;
          order = normalizeBackendOrder(response.data, summary, customer, session);
          if (!order || !backendOrderId) {
            applyInventoryDelta(summary.items, 1);
            return { ok: false, error: "Backend checkout did not return a valid order." };
          }
          backendMessage = "Created from backend checkout response.";
          if (backendOrderId && normalizedPaymentMethod === "EWallet") {
            const paymentResult = await window.storefrontApi.authorizePayment(backendOrderId);
            if (paymentResult.ok) {
              paymentReference = paymentResult.data?.reference || null;
              const markPaidResult = await window.storefrontApi.payOrder(backendOrderId);
              if (markPaidResult.ok) {
                order = normalizeBackendOrder(markPaidResult.data, summary, customer, session, paymentReference) || order;
                backendMessage = paymentReference
                  ? `Backend order paid via E-Wallet (${paymentReference}).`
                  : "Backend order paid via E-Wallet.";
              } else {
                backendMessage = `Payment authorized${paymentReference ? ` (${paymentReference})` : ""}, but mark-paid failed: ${markPaidResult.error}`;
              }
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
          order.paymentReference = paymentReference;
        } else if (response.error !== "API unavailable") {
          applyInventoryDelta(summary.items, 1);
          return { ok: false, error: `Backend checkout failed: ${response.error}` };
        }
      } catch (error) {
        applyInventoryDelta(summary.items, 1);
        return { ok: false, error: `Backend checkout failed: ${error instanceof Error ? error.message : "Unknown error"}` };
      }
    }

    if (!order) {
      order = createLocalOrderRecord(
        normalizedPaymentMethod,
        summary,
        customer,
        session,
        "Local demo",
        paymentReference
      );
    }
    persistOrderRecord(order);
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
    loadProductsWithApi,
    getVoucher,
    updateProduct,
    updateVariantStock,
    createStaffProductWithApi,
    addStaffProductVariantWithApi,
    setStaffInventoryWithApi,
    restockStaffInventoryWithApi,
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
    loadStaffOrdersWithApi,
    applyStaffOrderActionWithApi,
    registerCustomer
  };
})();
