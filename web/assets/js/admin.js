(async function renderAdminDashboard() {
  function isAdminRole(role) {
    return ["Admin", "Manager", "2", "3"].includes(String(role));
  }

  function escapeHtml(value) {
    return String(value)
      .replaceAll("&", "&amp;")
      .replaceAll("<", "&lt;")
      .replaceAll(">", "&gt;")
      .replaceAll('"', "&quot;")
      .replaceAll("'", "&#39;");
  }

  const session = window.storefrontState.getSession();
  const sessionElement = document.getElementById("admin-session");
  const feedbackElement = document.getElementById("admin-feedback");
  const overviewMetricsElement = document.getElementById("admin-overview-metrics");
  const createProductForm = document.getElementById("admin-create-product-form");
  const addVariantForm = document.getElementById("admin-add-variant-form");
  const variantProductSelect = document.getElementById("admin-variant-product");
  const productListElement = document.getElementById("admin-product-list");
  const voucherEditorElement = document.getElementById("admin-voucher-editor");
  const accountForm = document.getElementById("admin-account-form");
  const accountListElement = document.getElementById("admin-account-list");
  const reportBodyElement = document.getElementById("admin-report-body");
  let productsCache = [];
  let accountsCache = [];

  if (!session || !isAdminRole(session.role)) {
    window.location.href = "login.html";
    return;
  }

  if (sessionElement) {
    sessionElement.textContent = `Signed in as ${session.displayName || session.username} (${session.role}).`;
  }

  function setFeedback(ok, message) {
    feedbackElement.dataset.state = ok ? "success" : "error";
    feedbackElement.textContent = message;
  }

  function mergeImageText(imageText, uploadedImages) {
    const lines = String(imageText || "")
      .split(/\r?\n/g)
      .map((entry) => String(entry || "").trim())
      .filter(Boolean);
    return Array.from(new Set([...lines, ...(uploadedImages || [])])).join("\n");
  }

  async function readFilesAsDataUrls(fileInput) {
    const files = Array.from(fileInput?.files || []);
    return Promise.all(files.map((file) => new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = () => resolve(String(reader.result || ""));
      reader.onerror = () => reject(new Error(`Could not read ${file.name}`));
      reader.readAsDataURL(file);
    })));
  }

  async function loadProducts() {
    if (window.storefrontState.loadProductsWithApi) {
      const result = await window.storefrontState.loadProductsWithApi();
      productsCache = result.products;
      if (!result.ok && result.error !== "API unavailable") {
        setFeedback(false, result.error);
      }
      return;
    }
    productsCache = window.storefrontState.getProducts();
  }

  async function loadAccounts() {
    if (window.storefrontState.loadManagedAccountsWithApi) {
      const result = await window.storefrontState.loadManagedAccountsWithApi();
      accountsCache = result.accounts;
      if (!result.ok && result.error !== "API unavailable") {
        setFeedback(false, result.error);
      }
      return;
    }
    accountsCache = window.storefrontState.getManagedAccounts();
  }

  function renderOverview(report, accountCount) {
    const metrics = [
      { label: "Active Products", value: report.activeProducts },
      { label: "Total Orders", value: report.totalOrders },
      { label: "Live Accounts", value: accountCount },
      { label: "Net Revenue", value: window.storefrontState.formatMoney(report.revenueMinor) }
    ];

    overviewMetricsElement.innerHTML = metrics.map((metric) => `
      <article>
        <span>${escapeHtml(metric.label)}</span>
        <p>${escapeHtml(metric.value)}</p>
      </article>
    `).join("");
  }

  function renderVariantProductSelect() {
    if (productsCache.length === 0) {
      variantProductSelect.innerHTML = '<option value="">Create a product first</option>';
      variantProductSelect.disabled = true;
      return;
    }
    variantProductSelect.disabled = false;
    variantProductSelect.innerHTML = productsCache.map((product) => `
      <option value="${escapeHtml(product.productId)}">${escapeHtml(product.name)} / ${escapeHtml(product.productId)}</option>
    `).join("");
  }

  async function saveProductCard(card) {
    const productId = String(card.dataset.productId || "");
    const currentProduct = window.storefrontState.getProduct(productId);
    const name = card.querySelector('[name="name"]').value.trim();
    const category = card.querySelector('[name="category"]').value.trim();
    const collection = String(currentProduct?.collection || "").trim();
    const priceMinor = Number(card.querySelector('[name="priceMinor"]').value);
    const status = card.querySelector('[name="status"]').value;
    const description = card.querySelector('[name="description"]').value.trim();
    const imageUrlsInput = card.querySelector('[name="imageUrlsText"]');
    const imageFilesInput = card.querySelector('[name="imageFiles"]');
    const uploadedImages = await readFilesAsDataUrls(imageFilesInput);
    const imageUrlsText = mergeImageText(imageUrlsInput?.value, uploadedImages);
    const stockInputs = Array.from(card.querySelectorAll("[data-variant-stock]"));
    const stockDrafts = stockInputs.map((input) => ({
      variantId: String(input.dataset.variantId || ""),
      onHand: Number(input.value)
    }));

    if (stockDrafts.some((draft) => !draft.variantId || !Number.isFinite(draft.onHand) || draft.onHand < 0)) {
      setFeedback(false, "Stock must be zero or greater for every variant.");
      return;
    }

    const productResult = window.storefrontState.updateProductWithApi
      ? await window.storefrontState.updateProductWithApi(productId, {
        name,
        category,
        collection,
        priceMinor,
        status,
        description
      })
      : window.storefrontState.updateProduct(productId, {
        name,
        category,
        collection,
        priceMinor,
        status,
        description
      });
    if (!productResult.ok) {
      setFeedback(false, productResult.error);
      return;
    }

    let inventorySavedToBackend = false;
    for (const draft of stockDrafts) {
      const stockResult = window.storefrontState.setStaffInventoryWithApi
        ? await window.storefrontState.setStaffInventoryWithApi({
          productId,
          variantId: draft.variantId,
          onHand: draft.onHand,
          reserved: 0
        })
        : window.storefrontState.updateVariantStock(productId, draft.variantId, draft.onHand);
      if (!stockResult.ok) {
        setFeedback(false, stockResult.error);
        return;
      }
      inventorySavedToBackend = inventorySavedToBackend || stockResult.source === "api";
    }

    const imageResult = await window.storefrontState.saveProductImagesWithApi(productId, {
      imageUrlsText,
      images: uploadedImages
    });
    if (!imageResult.ok) {
      setFeedback(false, imageResult.error);
      return;
    }

    if (imageUrlsInput) {
      imageUrlsInput.value = imageUrlsText;
    }
    if (imageFilesInput) {
      imageFilesInput.value = "";
    }

    setFeedback(true, `${productResult.product.name} updated successfully.`);
    await render();
  }

  function renderProductList(products) {
    productListElement.innerHTML = products.map((product) => `
      <article class="payment-card admin-product-card" data-product-id="${escapeHtml(product.productId)}">
        <div class="admin-card-head">
          <div>
            <p class="summary-card-title">${escapeHtml(product.name)}</p>
            <p class="payment-meta">${escapeHtml(product.category)} / ${product.variants.length} variants</p>
          </div>
          <div class="order-history-status">
            <span>${escapeHtml(product.status)}</span>
            <span>${window.storefrontState.formatMoney(product.priceMinor)}</span>
          </div>
        </div>

        <div class="admin-field-grid">
          <label class="admin-field">
            <span>Name</span>
            <input name="name" type="text" value="${escapeHtml(product.name)}">
          </label>
          <label class="admin-field">
            <span>Category</span>
            <input name="category" type="text" value="${escapeHtml(product.category)}">
          </label>
          <label class="admin-field">
            <span>Price (VND)</span>
            <input name="priceMinor" type="number" min="0" step="1000" value="${escapeHtml(product.priceMinor)}">
          </label>
          <label class="admin-field">
            <span>Status</span>
            <select name="status">
              ${["Active", "Draft", "Discontinued"].map((status) => `
                <option value="${status}" ${status === product.status ? "selected" : ""}>${status}</option>
              `).join("")}
            </select>
          </label>
          <label class="admin-field admin-field-full">
            <span>Description</span>
            <textarea name="description" rows="4">${escapeHtml(product.description)}</textarea>
          </label>
          <label class="admin-field admin-field-full">
            <span>Image URLs (one per line)</span>
            <textarea name="imageUrlsText" rows="4">${escapeHtml((product.images || []).join("\n"))}</textarea>
          </label>
          <label class="admin-field admin-field-full">
            <span>Upload images from device</span>
            <input name="imageFiles" type="file" accept="image/*" multiple>
          </label>
        </div>

        <div class="admin-image-preview-list">
          ${(product.images || []).map((image, index) => `
            <img src="${escapeHtml(image)}" alt="${escapeHtml(product.name)} image ${index + 1}">
          `).join("")}
        </div>

        <div class="admin-variant-list">
          ${product.variants.map((variant) => `
            <label class="admin-inline-field">
              <span>${escapeHtml(variant.color)} / Size ${escapeHtml(variant.size)} / ${escapeHtml(variant.sku)}</span>
              <input
                type="number"
                min="0"
                step="1"
                value="${escapeHtml(variant.stockQuantity)}"
                data-variant-stock
                data-variant-id="${escapeHtml(variant.variantId)}">
            </label>
          `).join("")}
        </div>

        <div class="admin-actions">
          <button class="button primary compact" type="button" data-save-product>Save Product</button>
        </div>
      </article>
    `).join("");

    productListElement.querySelectorAll("[data-save-product]").forEach((button) => {
      button.addEventListener("click", async () => {
        const card = button.closest("[data-product-id]");
        if (!card) {
          return;
        }
        button.disabled = true;
        try {
          await saveProductCard(card);
        } finally {
          button.disabled = false;
        }
      });
    });
  }

  function renderVoucherEditor(voucher) {
    voucherEditorElement.innerHTML = `
      <form id="admin-voucher-form" class="admin-field-grid admin-form">
        <label class="admin-field">
          <span>Voucher Code</span>
          <input name="code" type="text" value="${escapeHtml(voucher.code)}">
        </label>
        <label class="admin-field">
          <span>Discount Rate (%)</span>
          <input name="ratePercent" type="number" min="0" max="100" step="1" value="${escapeHtml(Math.round(voucher.rate * 100))}">
        </label>
        <label class="admin-field">
          <span>Min Order (VND)</span>
          <input name="minOrderMinor" type="number" min="0" step="1000" value="${escapeHtml(voucher.minOrderMinor)}">
        </label>
        <label class="admin-field">
          <span>Max Discount (VND)</span>
          <input name="maxDiscountMinor" type="number" min="0" step="1000" value="${escapeHtml(voucher.maxDiscountMinor)}">
        </label>
        <label class="admin-field">
          <span>Availability</span>
          <select name="isActive">
            <option value="true" ${voucher.isActive ? "selected" : ""}>Active</option>
            <option value="false" ${voucher.isActive ? "" : "selected"}>Paused</option>
          </select>
        </label>
        <div class="admin-actions">
          <button class="button primary compact" type="submit">Save Voucher</button>
        </div>
      </form>
    `;

    const form = document.getElementById("admin-voucher-form");
    form.addEventListener("submit", (event) => {
      event.preventDefault();
      const formData = new FormData(form);
      const result = window.storefrontState.updateVoucher({
        code: String(formData.get("code") || "").trim(),
        rate: Number(formData.get("ratePercent") || 0) / 100,
        minOrderMinor: Number(formData.get("minOrderMinor") || 0),
        maxDiscountMinor: Number(formData.get("maxDiscountMinor") || 0),
        isActive: String(formData.get("isActive")) === "true"
      });
      if (!result.ok) {
        setFeedback(false, result.error);
        return;
      }
      setFeedback(true, `Voucher ${result.voucher.code} updated successfully.`);
      render();
    });
  }

  function renderAccountList(accounts) {
    accountListElement.innerHTML = accounts.map((account) => `
      <article class="payment-card admin-product-card" data-account-id="${escapeHtml(account.accountId)}">
        <div class="admin-card-head">
          <div>
            <p class="summary-card-title">${escapeHtml(account.displayName)}</p>
            <p class="payment-meta">${escapeHtml(account.username)} / ${escapeHtml(account.role)}</p>
          </div>
          <div class="order-history-status">
            <span>${escapeHtml(account.status)}</span>
            <span>${escapeHtml(account.position || (account.customerId ? "Customer" : "Account"))}</span>
          </div>
        </div>

        <div class="admin-field-grid">
          <label class="admin-field">
            <span>Reset Password</span>
            <input name="password" type="text" placeholder="new-password">
          </label>
        </div>

        <div class="admin-actions">
          <button class="button compact ${account.status === "Active" ? "" : "primary"}" type="button" data-toggle-status>
            ${account.status === "Active" ? "Lock Account" : "Unlock Account"}
          </button>
          <button class="button primary compact" type="button" data-reset-password>Reset Password</button>
        </div>
      </article>
    `).join("");

    accountListElement.querySelectorAll("[data-toggle-status]").forEach((button) => {
      button.addEventListener("click", async () => {
        const card = button.closest("[data-account-id]");
        const accountId = String(card?.dataset.accountId || "");
        const account = accounts.find((entry) => entry.accountId === accountId);
        if (!account) {
          return;
        }
        const nextStatus = account.status === "Active" ? "Locked" : "Active";
        const result = window.storefrontState.setManagedAccountStatusWithApi
          ? await window.storefrontState.setManagedAccountStatusWithApi(accountId, nextStatus)
          : window.storefrontState.setManagedAccountStatus(accountId, nextStatus);
        setFeedback(result.ok, result.ok
          ? `${account.username} is now ${nextStatus}.`
          : result.error);
        await render();
      });
    });

    accountListElement.querySelectorAll("[data-reset-password]").forEach((button) => {
      button.addEventListener("click", async () => {
        const card = button.closest("[data-account-id]");
        const accountId = String(card?.dataset.accountId || "");
        const password = String(card?.querySelector('[name="password"]')?.value || "").trim();
        const result = window.storefrontState.resetManagedAccountPasswordWithApi
          ? await window.storefrontState.resetManagedAccountPasswordWithApi(accountId, password)
          : window.storefrontState.resetManagedAccountPassword(accountId, password);
        setFeedback(result.ok, result.ok
          ? "Password reset completed."
          : result.error);
        if (result.ok) {
          const input = card?.querySelector('[name="password"]');
          if (input) {
            input.value = "";
          }
        }
      });
    });
  }

  function renderReportBody(report) {
    const bestSelling = report.bestSelling
      ? `<p><strong>${escapeHtml(report.bestSelling.productName)}</strong></p><p class="payment-meta">${escapeHtml(report.bestSelling.quantity)} units ordered</p>`
      : '<p class="payment-meta">No completed order volume yet.</p>';

    const lowStock = report.lowStockVariants.length > 0
      ? report.lowStockVariants.map((item) => `
          <article class="admin-report-item">
            <p><strong>${escapeHtml(item.productName)}</strong></p>
            <p class="payment-meta">${escapeHtml(item.variantLabel)} / ${escapeHtml(item.stockQuantity)} left</p>
          </article>
        `).join("")
      : '<p class="payment-meta">No low-stock variants right now.</p>';

    const recentOrders = report.recentOrders.length > 0
      ? report.recentOrders.map((order) => `
          <article class="admin-report-item">
            <p><strong>${escapeHtml(order.orderNumber)}</strong></p>
            <p class="payment-meta">${escapeHtml(order.customerName)} / ${escapeHtml(order.orderStatus)} / ${window.storefrontState.formatMoney(order.totalMinor)}</p>
          </article>
        `).join("")
      : '<p class="payment-meta">No orders yet.</p>';

    reportBodyElement.innerHTML = `
      <div class="admin-report-grid">
        <article class="admin-report-card">
          <p class="summary-card-title">Voucher Snapshot</p>
          <p><strong>${escapeHtml(report.voucher.code)}</strong></p>
          <p class="payment-meta">${report.voucher.isActive ? "Active" : "Paused"} / ${escapeHtml(Math.round(report.voucher.rate * 100))}% / max ${window.storefrontState.formatMoney(report.voucher.maxDiscountMinor)}</p>
        </article>
        <article class="admin-report-card">
          <p class="summary-card-title">Best Selling</p>
          ${bestSelling}
        </article>
        <article class="admin-report-card">
          <p class="summary-card-title">Payment Summary</p>
          <p><strong>${escapeHtml(report.paidOrders)}</strong> paid orders</p>
          <p class="payment-meta">${escapeHtml(report.refundedOrders)} refunded orders</p>
        </article>
      </div>

      <div class="admin-report-grid admin-report-grid-wide">
        <article class="admin-report-card">
          <p class="summary-card-title">Low Stock</p>
          <div class="admin-report-list">${lowStock}</div>
        </article>
        <article class="admin-report-card">
          <p class="summary-card-title">Recent Orders</p>
          <div class="admin-report-list">${recentOrders}</div>
        </article>
      </div>
    `;
  }

  function attachCreateForms() {
    createProductForm.addEventListener("submit", async (event) => {
      event.preventDefault();
      const formData = new FormData(createProductForm);
      const uploadedImages = await readFilesAsDataUrls(createProductForm.querySelector('[name="imageFiles"]'));
      const imageUrlsText = mergeImageText(formData.get("imageUrlsText"), uploadedImages);
      const submitButton = createProductForm.querySelector('button[type="submit"]');
      submitButton.disabled = true;
      const result = await window.storefrontState.createStaffProductWithApi({
        productId: String(formData.get("productId") || "").trim(),
        name: String(formData.get("name") || "").trim(),
        category: String(formData.get("category") || "Accessories"),
        status: String(formData.get("status") || "Draft"),
        collection: String(formData.get("collection") || "").trim(),
        description: String(formData.get("description") || "").trim(),
        imageUrlsText,
        images: uploadedImages
      });
      submitButton.disabled = false;
      setFeedback(result.ok, result.ok
        ? `${result.product.name} created successfully.`
        : result.error);
      if (result.ok) {
        createProductForm.reset();
        await render();
      }
    });

    addVariantForm.addEventListener("submit", async (event) => {
      event.preventDefault();
      const formData = new FormData(addVariantForm);
      const productId = String(formData.get("productId") || "");
      if (!productId) {
        setFeedback(false, "Choose a product before adding a variant.");
        return;
      }

      const submitButton = addVariantForm.querySelector('button[type="submit"]');
      submitButton.disabled = true;
      const result = await window.storefrontState.addStaffProductVariantWithApi(productId, {
        variantId: String(formData.get("variantId") || "").trim(),
        sku: String(formData.get("sku") || "").trim(),
        size: String(formData.get("size") || "").trim(),
        color: String(formData.get("color") || "").trim(),
        priceMinor: Number(formData.get("priceMinor") || 0),
        stockQuantity: Number(formData.get("stockQuantity") || 0)
      });
      submitButton.disabled = false;
      setFeedback(result.ok, result.ok ? "Variant added." : result.error);
      if (result.ok) {
        addVariantForm.reset();
        await render();
      }
    });

    accountForm.addEventListener("submit", async (event) => {
      event.preventDefault();
      const formData = new FormData(accountForm);
      const result = window.storefrontState.createManagedAccountWithApi
        ? await window.storefrontState.createManagedAccountWithApi({
            role: String(formData.get("role") || "Staff"),
            fullName: String(formData.get("fullName") || "").trim(),
            username: String(formData.get("username") || "").trim(),
            password: String(formData.get("password") || "").trim()
          })
        : window.storefrontState.createManagedAccount({
            role: String(formData.get("role") || "Staff"),
            fullName: String(formData.get("fullName") || "").trim(),
            username: String(formData.get("username") || "").trim(),
            password: String(formData.get("password") || "").trim()
          });
      setFeedback(result.ok, result.ok
        ? `${result.account.username} created successfully.`
        : result.error);
      if (result.ok) {
        accountForm.reset();
        await render();
      }
    });
  }

  async function render() {
    await loadProducts();
    await loadAccounts();
    // Ensure admin view merges live backend orders into local storage when available
    if (window.storefrontState.loadStaffOrdersWithApi) {
      const ordersResult = await window.storefrontState.loadStaffOrdersWithApi();
      if (!ordersResult.ok && ordersResult.error !== "API unavailable") {
        setFeedback(false, ordersResult.error);
      }
    }
    const voucher = window.storefrontState.getVoucher();
    const report = window.storefrontState.getAdminReport();
    renderOverview(report, accountsCache.length);
    renderVariantProductSelect();
    renderProductList(productsCache);
    renderVoucherEditor(voucher);
    renderAccountList(accountsCache);
    renderReportBody(report);
  }

  attachCreateForms();
  await render();
})();
