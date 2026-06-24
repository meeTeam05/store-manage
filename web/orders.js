(async function renderOrdersPage() {
  function isCustomerRole(role) {
    return role === "Customer" || role === 0;
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
  if (!session) {
    window.location.href = "login.html?returnTo=orders.html";
    return;
  }

  if (!isCustomerRole(session.role)) {
    window.location.href = "index.html";
    return;
  }

  const sessionElement = document.getElementById("orders-session");
  const feedbackElement = document.getElementById("orders-feedback");
  const emptyState = document.getElementById("orders-empty");
  const ordersList = document.getElementById("orders-list");
  let ordersCache = [];
  let ordersSource = "local";

  if (sessionElement) {
    sessionElement.textContent = `Signed in as ${session.displayName || session.customerName || session.username}.`;
  }

  function renderOrders(orders) {
    if (orders.length === 0) {
      emptyState.hidden = false;
      ordersList.innerHTML = "";
      return;
    }

    emptyState.hidden = true;
    ordersList.innerHTML = orders.map((order) => {
      const canCancel = window.storefrontState.canCancelOrder
        ? window.storefrontState.canCancelOrder(order)
        : ["AwaitingPayment", "Paid"].includes(order.orderStatus);
      return `
        <article class="payment-card order-history-card">
          <div class="order-history-head">
            <div>
              <p class="summary-card-title">${escapeHtml(order.orderNumber)}</p>
              <p class="payment-meta">${escapeHtml(new Date(order.createdAtIso).toLocaleString("vi-VN"))}</p>
            </div>
            <div class="order-history-status">
              <span>${escapeHtml(order.orderStatus)}</span>
              <span>${escapeHtml(order.paymentStatus)}</span>
              <span>${escapeHtml(order.shippingStatus)}</span>
            </div>
          </div>

          <div class="info-grid order-history-grid">
            <article>
              <span>Payment</span>
              <p>${escapeHtml(order.paymentMethod)}${order.paymentReference ? ` / ${escapeHtml(order.paymentReference)}` : ""}</p>
            </article>
            <article>
              <span>Tracking</span>
              <p>${escapeHtml(order.trackingCode)}</p>
            </article>
            <article>
              <span>Voucher</span>
              <p>${escapeHtml(order.voucherCode || "No voucher")}</p>
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
            ${canCancel
              ? `<button class="button compact" type="button" data-cancel-order="${escapeHtml(order.orderId)}">Cancel Order</button>`
              : '<p class="payment-meta staff-order-note">This order can no longer be cancelled.</p>'}
          </div>
        </article>
      `;
    }).join("");

    ordersList.querySelectorAll("[data-cancel-order]").forEach((button) => {
      button.addEventListener("click", async () => {
        const orderId = String(button.dataset.cancelOrder || "");
        button.disabled = true;
        const result = window.storefrontState.cancelCustomerOrderWithApi
          ? await window.storefrontState.cancelCustomerOrderWithApi(orderId)
          : window.storefrontState.cancelCustomerOrder(orderId);
        if (feedbackElement) {
          feedbackElement.dataset.state = result.ok ? "success" : "error";
          feedbackElement.textContent = result.ok
            ? `${result.order.orderNumber} cancelled successfully.`
            : result.error;
        }
        await render();
      });
    });
  }

  async function loadOrders() {
    if (!window.storefrontState.loadCustomerOrdersWithApi) {
      ordersCache = window.storefrontState.getCustomerOrders();
      ordersSource = "local";
      return;
    }

    const result = await window.storefrontState.loadCustomerOrdersWithApi();
    ordersCache = result.orders;
    ordersSource = result.source;
    if (feedbackElement && !result.ok && result.error !== "API unavailable") {
      feedbackElement.dataset.state = "error";
      feedbackElement.textContent = result.error;
    }
  }

  async function render() {
    await loadOrders();
    renderOrders(ordersCache);
    if (feedbackElement && !feedbackElement.textContent) {
      feedbackElement.dataset.state = "success";
      feedbackElement.textContent = ordersSource === "api"
        ? "Loaded your current backend orders."
        : "Showing local session orders.";
    }
  }

  await render();
})();
