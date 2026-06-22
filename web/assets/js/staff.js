(function renderStaffDashboard() {
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
  const filters = ["All", "AwaitingPayment", "Paid", "Packed", "Shipped", "Delivered", "Completed", "Cancelled"];
  let activeFilter = "All";

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
        render();
      });
    });
  }

  function renderOrderList(orders) {
    const visibleOrders = activeFilter === "All"
      ? orders
      : orders.filter((entry) => entry.orderStatus === activeFilter);

    if (visibleOrders.length === 0) {
      emptyElement.hidden = false;
      emptyElement.textContent = orders.length === 0
        ? "No local orders yet. Place a customer test order first."
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
      button.addEventListener("click", () => {
        const orderId = String(button.dataset.orderId || "");
        const action = String(button.dataset.orderAction || "");
        const result = window.storefrontState.applyStaffOrderAction(orderId, action);
        feedbackElement.dataset.state = result.ok ? "success" : "error";
        feedbackElement.textContent = result.ok
          ? `${result.order.orderNumber} updated: ${actionLabel(action)}.`
          : result.error;
        render();
      });
    });
  }

  function render() {
    const orders = window.storefrontState.getOrders();
    renderMetrics(orders);
    renderFilters();
    renderOrderList(orders);
  }

  render();
})();
