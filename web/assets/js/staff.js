(async function renderStaffDashboard() {
  function isStaffRole(role) {
    return ["Staff", "1"].includes(String(role));
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

  function orderActionsFor(order) {
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

  function returnActionsFor(request) {
    switch (request.status) {
      case "Requested":
        return [
          { id: "approve", label: "Approve", primary: true },
          { id: "reject", label: "Reject", primary: false }
        ];
      case "Approved":
        return [{ id: "restock", label: "Restock", primary: true }];
      case "Restocked":
        return [{ id: "refund", label: "Refund", primary: true }];
      case "Rejected":
      case "Refunded":
        return [{ id: "close", label: "Close Case", primary: false }];
      default:
        return [];
    }
  }

  function returnActionNeedsNote(actionId) {
    return ["approve", "reject", "restock", "close"].includes(actionId);
  }

  function returnActionNeedsRefundReference(actionId) {
    return actionId === "refund";
  }

  const session = window.storefrontState.getSession();
  const sessionElement = document.getElementById("staff-session");
  const metricsElement = document.getElementById("staff-order-metrics");
  const orderFiltersElement = document.getElementById("staff-order-filters");
  const orderFeedbackElement = document.getElementById("staff-order-feedback");
  const orderEmptyElement = document.getElementById("staff-order-empty");
  const orderListElement = document.getElementById("staff-order-list");
  const returnFiltersElement = document.getElementById("staff-return-filters");
  const returnFeedbackElement = document.getElementById("staff-return-feedback");
  const returnEmptyElement = document.getElementById("staff-return-empty");
  const returnListElement = document.getElementById("staff-return-list");

  const orderFilters = ["All", "AwaitingPayment", "Paid", "Packed", "Shipped", "Delivered", "Completed", "Cancelled"];
  const returnFilters = ["All", "Requested", "Approved", "Rejected", "Restocked", "Refunded", "Closed"];
  let activeOrderFilter = "All";
  let activeReturnFilter = "All";
  let ordersCache = [];
  let returnsCache = [];
  let orderSource = "local";
  let returnSource = "local";

  if (!session || !isStaffRole(session.role)) {
    window.location.href = "login.html";
    return;
  }

  if (sessionElement) {
    sessionElement.textContent = `Signed in as ${session.displayName || session.username} (${session.role}).`;
  }

  function renderMetrics(orders, returns) {
    const items = [
      { label: "Open Orders", value: orders.filter((entry) => !["Completed", "Cancelled"].includes(entry.orderStatus)).length },
      { label: "Awaiting Payment", value: orders.filter((entry) => entry.orderStatus === "AwaitingPayment").length },
      { label: "In Transit", value: orders.filter((entry) => entry.orderStatus === "Shipped").length },
      { label: "Open Returns", value: returns.filter((entry) => !["Closed", "Rejected"].includes(entry.status)).length }
    ];
    metricsElement.innerHTML = items.map((item) => `
      <article>
        <span>${escapeHtml(item.label)}</span>
        <p>${escapeHtml(item.value)}</p>
      </article>
    `).join("");
  }

  function renderOrderFilters() {
    orderFiltersElement.innerHTML = orderFilters.map((filter) => `
      <button class="chip ${filter === activeOrderFilter ? "is-active" : ""}" type="button" data-order-filter="${filter}">
        ${filter === "All" ? "All Orders" : escapeHtml(filter)}
      </button>
    `).join("");

    orderFiltersElement.querySelectorAll("[data-order-filter]").forEach((button) => {
      button.addEventListener("click", () => {
        activeOrderFilter = String(button.dataset.orderFilter || "All");
        renderOrderFilters();
        renderOrders();
      });
    });
  }

  function renderReturnFilters() {
    returnFiltersElement.innerHTML = returnFilters.map((filter) => `
      <button class="chip ${filter === activeReturnFilter ? "is-active" : ""}" type="button" data-return-filter="${filter}">
        ${filter === "All" ? "All Returns" : escapeHtml(filter)}
      </button>
    `).join("");

    returnFiltersElement.querySelectorAll("[data-return-filter]").forEach((button) => {
      button.addEventListener("click", () => {
        activeReturnFilter = String(button.dataset.returnFilter || "All");
        renderReturnFilters();
        renderReturns();
      });
    });
  }

  function renderOrders() {
    const visibleOrders = activeOrderFilter === "All"
      ? ordersCache
      : ordersCache.filter((entry) => entry.orderStatus === activeOrderFilter);

    if (visibleOrders.length === 0) {
      orderEmptyElement.hidden = false;
      orderEmptyElement.textContent = ordersCache.length === 0
        ? "No orders yet."
        : `No orders in ${activeOrderFilter}.`;
      orderListElement.innerHTML = "";
      return;
    }

    orderEmptyElement.hidden = true;
    orderListElement.innerHTML = visibleOrders.map((order) => {
      const actions = orderActionsFor(order);
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

    orderListElement.querySelectorAll("[data-order-action]").forEach((button) => {
      button.addEventListener("click", async () => {
        const orderId = String(button.dataset.orderId || "");
        const action = String(button.dataset.orderAction || "");
        button.disabled = true;
        const result = window.storefrontState.applyStaffOrderActionWithApi
          ? await window.storefrontState.applyStaffOrderActionWithApi(orderId, action)
          : window.storefrontState.applyStaffOrderAction(orderId, action);
        orderFeedbackElement.dataset.state = result.ok ? "success" : "error";
        orderFeedbackElement.textContent = result.ok
          ? `Updated ${result.order.orderNumber} successfully.`
          : result.error;
        await render();
      });
    });
  }

  function renderReturns() {
    const orderById = new Map(ordersCache.map((order) => [order.orderId, order]));
    const visibleReturns = activeReturnFilter === "All"
      ? returnsCache
      : returnsCache.filter((entry) => entry.status === activeReturnFilter);

    if (visibleReturns.length === 0) {
      returnEmptyElement.hidden = false;
      returnEmptyElement.textContent = returnsCache.length === 0
        ? "No return requests yet."
        : `No return requests in ${activeReturnFilter}.`;
      returnListElement.innerHTML = "";
      return;
    }

    returnEmptyElement.hidden = true;
    returnListElement.innerHTML = visibleReturns.map((request) => {
      const order = orderById.get(request.orderId) || null;
      const actions = returnActionsFor(request);
      const needsNote = actions.some((action) => returnActionNeedsNote(action.id));
      const needsRefundReference = actions.some((action) => returnActionNeedsRefundReference(action.id));
      return `
        <article class="payment-card order-history-card staff-order-card">
          <div class="staff-order-head">
            <div>
              <p class="summary-card-title">${escapeHtml(request.returnId)}</p>
              <p class="payment-meta">${escapeHtml(request.productName)} / Qty ${escapeHtml(request.quantity)}</p>
              <p class="payment-meta">${escapeHtml(order?.orderNumber || request.orderId)} / ${formatDate(request.createdAtIso)}</p>
            </div>
            <div class="order-history-status">
              <span>${escapeHtml(request.status)}</span>
              <span>${escapeHtml(order?.customerName || request.customerId || "Customer")}</span>
            </div>
          </div>

          <div class="info-grid order-history-grid">
            <article>
              <span>Reason</span>
              <p>${escapeHtml(request.reason)}</p>
            </article>
            <article>
              <span>Variant</span>
              <p>${escapeHtml(request.variantId || "N/A")}</p>
            </article>
            <article>
              <span>Order</span>
              <p>${escapeHtml(order?.orderStatus || "Unknown")}</p>
            </article>
          </div>

          <div class="staff-order-actions">
            ${needsNote ? `
              <label class="admin-field admin-field-full">
                <span>Staff Note</span>
                <textarea rows="2" placeholder="Reason, update, or closure note" data-return-note></textarea>
              </label>
            ` : ""}
            ${needsRefundReference ? `
              <label class="admin-field">
                <span>Refund Reference</span>
                <input type="text" placeholder="BANK-REF-001" data-refund-reference>
              </label>
            ` : ""}
            ${actions.map((action) => `
              <button
                class="button compact ${action.primary ? "primary" : ""}"
                type="button"
                data-return-id="${escapeHtml(request.returnId)}"
                data-return-action="${action.id}">
                ${escapeHtml(action.label)}
              </button>
            `).join("")}
            ${actions.length === 0 ? '<p class="payment-meta staff-order-note">No further return action for this case.</p>' : ""}
          </div>
        </article>
      `;
    }).join("");

    returnListElement.querySelectorAll("[data-return-action]").forEach((button) => {
      button.addEventListener("click", async () => {
        const returnId = String(button.dataset.returnId || "");
        const action = String(button.dataset.returnAction || "");
        const card = button.closest(".staff-order-card");
        const note = String(card?.querySelector("[data-return-note]")?.value || "").trim();
        const refundReference = String(card?.querySelector("[data-refund-reference]")?.value || "").trim();
        button.disabled = true;
        const result = window.storefrontState.applyStaffReturnActionWithApi
          ? await window.storefrontState.applyStaffReturnActionWithApi(returnId, action, { note, refundReference })
          : window.storefrontState.applyStaffReturnAction(returnId, action, { note, refundReference });
        returnFeedbackElement.dataset.state = result.ok ? "success" : "error";
        returnFeedbackElement.textContent = result.ok
          ? `Updated return ${returnId} successfully.`
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
      orderFeedbackElement.dataset.state = "error";
      orderFeedbackElement.textContent = result.error;
    }
  }

  async function loadReturns() {
    if (!window.storefrontState.loadStaffReturnsWithApi) {
      returnsCache = window.storefrontState.getStaffReturns ? window.storefrontState.getStaffReturns() : [];
      returnSource = "local";
      return;
    }

    const result = await window.storefrontState.loadStaffReturnsWithApi();
    returnsCache = result.returns;
    returnSource = result.source;
    if (!result.ok && result.error !== "API unavailable") {
      returnFeedbackElement.dataset.state = "error";
      returnFeedbackElement.textContent = result.error;
    }
  }

  async function render() {
    await loadOrders();
    await loadReturns();
    renderMetrics(ordersCache, returnsCache);
    renderOrderFilters();
    renderReturnFilters();
    renderOrders();
    renderReturns();

    if (orderFeedbackElement && !orderFeedbackElement.textContent) {
      orderFeedbackElement.dataset.state = "success";
      orderFeedbackElement.textContent = "Orders updated.";
    }
    if (returnFeedbackElement && !returnFeedbackElement.textContent) {
      returnFeedbackElement.dataset.state = "success";
      returnFeedbackElement.textContent = "Returns updated.";
    }
  }

  await render();
})();
