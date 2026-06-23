(async function renderStaffDashboard() {
  function isStaffRole(role) {
    return ["Staff", "Manager", "Admin", "1", "2", "3"].includes(String(role));
  }

  function escapeHtml(value) {
    return String(value)
      .replaceAll("&", "&amp;")
      .replaceAll("<", "&lt;")
      .replaceAll(">", "&gt;")
      .replaceAll('"', "&quot;")
      .replaceAll("'", "&#39;");
  }

  function formatDate(isoString) {
    return new Date(isoString).toLocaleString("vi-VN");
  }

  function actionButtonsFor(order) {
    switch (order.orderStatus) {
      case "AwaitingPayment":
        return [
          { id: "mark-paid", label: "Mark Paid", primary: true },
          { id: "cancel", label: "Cancel Order", primary: false }
        ];
      case "Paid":
        return [
          { id: "pack", label: "Pack Order", primary: true },
          { id: "cancel", label: "Cancel Order", primary: false }
        ];
      case "Packed":
        return [{ id: "ship", label: "Ship Order", primary: true }];
      case "Shipped":
        return [{ id: "deliver", label: "Mark Delivered", primary: true }];
      case "Delivered":
        return [{ id: "complete", label: "Complete Order", primary: true }];
      default:
        return [];
    }
  }

  function actionLabel(action) {
    return {
      "mark-paid": "mark paid",
      pack: "pack order",
      ship: "ship order",
      deliver: "mark delivered",
      complete: "complete order",
      cancel: "cancel order"
    }[action] || action;
  }

  const session = window.storefrontState.getSession();
  const sessionElement = document.getElementById("staff-session");
  const metricsElement = document.getElementById("staff-order-metrics");
  const filtersElement = document.getElementById("staff-order-filters");
  const feedbackElement = document.getElementById("staff-order-feedback");
  const emptyElement = document.getElementById("staff-order-empty");
  const listElement = document.getElementById("staff-order-list");
  const catalogFeedbackElement = document.getElementById("staff-catalog-feedback");
  const productForm = document.getElementById("staff-product-form");
  const variantForm = document.getElementById("staff-variant-form");
  const variantProductSelect = document.getElementById("staff-variant-product");
  const inventoryListElement = document.getElementById("staff-inventory-list");
  const filters = ["All", "AwaitingPayment", "Paid", "Packed", "Shipped", "Delivered", "Completed", "Cancelled"];
  let activeFilter = "All";
  let ordersCache = [];
  let orderSource = "local";
  let productsCache = [];

  if (!session || !isStaffRole(session.role)) {
    window.location.href = "login.html";
    return;
  }

  if (sessionElement) {
    sessionElement.textContent = `Signed in as ${session.displayName || session.username} (${session.role}).`;
  }

  function renderMetrics(orders) {
    const metricItems = [
      { label: "Open Orders", value: orders.filter((entry) => !["Completed", "Cancelled"].includes(entry.orderStatus)).length },
      { label: "Awaiting Payment", value: orders.filter((entry) => entry.orderStatus === "AwaitingPayment").length },
      { label: "Ready To Pack", value: orders.filter((entry) => entry.orderStatus === "Paid").length },
      { label: "In Transit", value: orders.filter((entry) => entry.orderStatus === "Shipped").length }
    ];

    metricsElement.innerHTML = metricItems.map((item) => `
      <article>
        <span>${escapeHtml(item.label)}</span>
        <p>${escapeHtml(item.value)}</p>
      </article>
    `).join("");
  }

  function renderFilters() {
    filtersElement.innerHTML = filters.map((filter) => `
      <button
        class="chip ${filter === activeFilter ? "is-active" : ""}"
        type="button"
        data-filter="${filter}">
        ${filter === "All" ? "All Orders" : escapeHtml(filter)}
      </button>
    `).join("");

    filtersElement.querySelectorAll("[data-filter]").forEach((button) => {
      button.addEventListener("click", () => {
        activeFilter = String(button.dataset.filter || "All");
        renderOrderList(ordersCache);
        renderFilters();
      });
    });
  }

  function setCatalogFeedback(ok, message) {
    catalogFeedbackElement.dataset.state = ok ? "success" : "error";
    catalogFeedbackElement.textContent = message;
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

  function renderOrderList(orders) {
    const visibleOrders = activeFilter === "All"
      ? orders
      : orders.filter((entry) => entry.orderStatus === activeFilter);

    if (visibleOrders.length === 0) {
      emptyElement.hidden = false;
      emptyElement.textContent = orders.length === 0
        ? "No orders yet. Place a customer test order first."
        : `No orders in ${activeFilter}.`;
      listElement.innerHTML = "";
      return;
    }

    emptyElement.hidden = true;
    listElement.innerHTML = visibleOrders.map((order) => {
      const actions = actionButtonsFor(order);
      return `
        <article class="payment-card order-history-card staff-order-card">
          <div class="staff-order-head">
            <div>
              <p class="summary-card-title">${escapeHtml(order.orderNumber)}</p>
              <p class="payment-meta">${escapeHtml(order.customerName)} / ${formatDate(order.createdAtIso)}</p>
              <p class="payment-meta">${escapeHtml(order.paymentMethod)} / ${escapeHtml(order.trackingCode)}</p>
            </div>
            <div class="order-history-status">
              <span>${escapeHtml(order.orderStatus)}</span>
              <span>${escapeHtml(order.paymentStatus)}</span>
              <span>${escapeHtml(order.shippingStatus)}</span>
            </div>
          </div>

          <div class="info-grid order-history-grid">
            <article>
              <span>Subtotal</span>
              <p>${window.storefrontState.formatMoney(order.subtotalMinor)}</p>
            </article>
            <article>
              <span>Discount</span>
              <p>${window.storefrontState.formatMoney(order.discountMinor)}</p>
            </article>
            <article>
              <span>Total</span>
              <p>${window.storefrontState.formatMoney(order.totalMinor)}</p>
            </article>
            <article>
              <span>Backend</span>
              <p>${escapeHtml(order.backendState || "Local demo")}</p>
            </article>
          </div>

          <div class="payment-items order-history-items">
            ${order.items.map((item) => `
              <article class="payment-item">
                <div>
                  <p><strong>${escapeHtml(item.productName)}</strong></p>
                  <p class="payment-meta">${escapeHtml(item.color)} / Size ${escapeHtml(item.size)} / Qty ${escapeHtml(item.quantity)}</p>
                </div>
                <p>${window.storefrontState.formatMoney(item.lineTotalMinor)}</p>
              </article>
            `).join("")}
          </div>

          <div class="staff-order-actions">
            ${actions.map((action) => `
              <button
                class="button compact ${action.primary ? "primary" : ""}"
                type="button"
                data-order-id="${escapeHtml(order.orderId)}"
                data-order-action="${action.id}">
                ${escapeHtml(action.label)}
              </button>
            `).join("")}
            ${actions.length === 0 ? '<p class="payment-meta staff-order-note">No further staff action for this order.</p>' : ""}
          </div>
        </article>
      `;
    }).join("");

    listElement.querySelectorAll("[data-order-action]").forEach((button) => {
      button.addEventListener("click", async () => {
        const orderId = String(button.dataset.orderId || "");
        const action = String(button.dataset.orderAction || "");
        button.disabled = true;
        const result = window.storefrontState.applyStaffOrderActionWithApi
          ? await window.storefrontState.applyStaffOrderActionWithApi(orderId, action)
          : window.storefrontState.applyStaffOrderAction(orderId, action);
        feedbackElement.dataset.state = result.ok ? "success" : "error";
        feedbackElement.textContent = result.ok
          ? `${result.order.orderNumber} updated: ${actionLabel(action)}.`
          : result.error;
        await render();
      });
    });
  }

  async function loadOrders() {
    if (!window.storefrontState.loadStaffOrdersWithApi) {
      ordersCache = window.storefrontState.getOrders();
      orderSource = "local";
      return;
    }

    const result = await window.storefrontState.loadStaffOrdersWithApi();
    ordersCache = result.orders;
    orderSource = result.source;
    if (!result.ok && result.error !== "API unavailable") {
      feedbackElement.dataset.state = "error";
      feedbackElement.textContent = result.error;
    }
  }

  async function loadProducts() {
    if (window.storefrontState.loadProductsWithApi) {
      const result = await window.storefrontState.loadProductsWithApi();
      productsCache = result.products;
      if (!result.ok && result.error !== "API unavailable") {
        setCatalogFeedback(false, result.error);
      }
      return;
    }
    productsCache = window.storefrontState.getProducts();
  }

  function renderProductSelect() {
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

  function renderInventoryList() {
    const variants = productsCache.flatMap((product) => product.variants.map((variant) => ({
      product,
      variant
    })));

    if (variants.length === 0) {
      inventoryListElement.innerHTML = `
        <article class="empty-state">
          <h3>No variants available</h3>
          <p>Create a product variant before updating inventory.</p>
        </article>
      `;
      return;
    }

    inventoryListElement.innerHTML = variants.map(({ product, variant }) => `
      <article class="payment-card admin-product-card staff-inventory-card">
        <div class="admin-card-head">
          <div>
            <p class="summary-card-title">${escapeHtml(product.name)}</p>
            <p class="payment-meta">${escapeHtml(variant.color)} / Size ${escapeHtml(variant.size)} / ${escapeHtml(variant.sku)}</p>
          </div>
          <div class="order-history-status">
            <span>${escapeHtml(product.status)}</span>
            <span>${escapeHtml(variant.stockQuantity)}</span>
          </div>
        </div>
        <div class="admin-field-grid admin-form">
          <label class="admin-field">
            <span>Set Available</span>
            <input type="number" min="0" step="1" value="${escapeHtml(variant.stockQuantity)}" data-stock-value>
          </label>
          <label class="admin-field">
            <span>Restock Qty</span>
            <input type="number" min="1" step="1" value="1" data-restock-value>
          </label>
          <div class="admin-actions">
            <button class="button primary compact" type="button" data-inventory-action="set" data-product-id="${escapeHtml(product.productId)}" data-variant-id="${escapeHtml(variant.variantId)}">Set Stock</button>
            <button class="button compact" type="button" data-inventory-action="restock" data-product-id="${escapeHtml(product.productId)}" data-variant-id="${escapeHtml(variant.variantId)}">Restock</button>
          </div>
        </div>
      </article>
    `).join("");

    inventoryListElement.querySelectorAll("[data-inventory-action]").forEach((button) => {
      button.addEventListener("click", async () => {
        const card = button.closest(".staff-inventory-card");
        const productId = String(button.dataset.productId || "");
        const variantId = String(button.dataset.variantId || "");
        const action = String(button.dataset.inventoryAction || "");
        button.disabled = true;

        const result = action === "set"
          ? await window.storefrontState.setStaffInventoryWithApi({
              productId,
              variantId,
              onHand: Number(card.querySelector("[data-stock-value]").value || 0),
              reserved: 0
            })
          : await window.storefrontState.restockStaffInventoryWithApi({
              productId,
              variantId,
              quantity: Number(card.querySelector("[data-restock-value]").value || 0)
            });

        setCatalogFeedback(result.ok, result.ok ? "Inventory updated." : result.error);
        await renderCatalog();
      });
    });
  }

  async function renderCatalog() {
    await loadProducts();
    renderProductSelect();
    renderInventoryList();
  }

  function attachCatalogForms() {
    productForm.addEventListener("submit", async (event) => {
      event.preventDefault();
      const formData = new FormData(productForm);
      const uploadedImages = await readFilesAsDataUrls(productForm.querySelector('[name="imageFiles"]'));
      const imageUrlsText = mergeImageText(formData.get("imageUrlsText"), uploadedImages);
      const submitButton = productForm.querySelector('button[type="submit"]');
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
      setCatalogFeedback(result.ok, result.ok
        ? (result.source === "api"
          ? `${result.product.name} created and image metadata saved to \`data/product_storefront.json\`.`
          : `${result.product.name} created for this local session.`)
        : result.error);
      if (result.ok) {
        productForm.reset();
        await renderCatalog();
      }
    });

    variantForm.addEventListener("submit", async (event) => {
      event.preventDefault();
      const formData = new FormData(variantForm);
      const productId = String(formData.get("productId") || "");
      if (!productId) {
        setCatalogFeedback(false, "Choose a product before adding a variant.");
        return;
      }

      const submitButton = variantForm.querySelector('button[type="submit"]');
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
      setCatalogFeedback(result.ok, result.ok ? "Variant added." : result.error);
      if (result.ok) {
        variantForm.reset();
        await renderCatalog();
      }
    });
  }

  async function render() {
    await loadOrders();
    renderMetrics(ordersCache);
    renderFilters();
    renderOrderList(ordersCache);
    if (feedbackElement && !feedbackElement.textContent) {
      feedbackElement.textContent = orderSource === "api"
        ? "Loaded live backend orders."
        : "Showing local fallback orders.";
    }
  }

  attachCatalogForms();
  await renderCatalog();
  await render();
})();
