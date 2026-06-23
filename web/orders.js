(function renderOrdersPage() {
  function isCustomerRole(role) {
    return role === "Customer" || role === 0;
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
  const emptyState = document.getElementById("orders-empty");
  const ordersList = document.getElementById("orders-list");
  const orders = window.storefrontState.getCustomerOrders();

  if (sessionElement) {
    sessionElement.textContent = `Signed in as ${session.displayName || session.customerName || session.username}.`;
  }

  if (orders.length === 0) {
    emptyState.hidden = false;
    ordersList.innerHTML = "";
    return;
  }

  emptyState.hidden = true;
  ordersList.innerHTML = orders.map((order) => `
    <article class="payment-card order-history-card">
      <div class="order-history-head">
        <div>
          <p class="summary-card-title">${order.orderNumber}</p>
          <p class="payment-meta">${new Date(order.createdAtIso).toLocaleString("vi-VN")}</p>
        </div>
        <div class="order-history-status">
          <span>${order.orderStatus}</span>
          <span>${order.paymentStatus}</span>
          <span>${order.shippingStatus}</span>
        </div>
      </div>

      <div class="info-grid order-history-grid">
        <article>
          <span>Payment</span>
          <p>${order.paymentMethod}${order.paymentReference ? ` / ${order.paymentReference}` : ""}</p>
        </article>
        <article>
          <span>Tracking</span>
          <p>${order.trackingCode}</p>
        </article>
        <article>
          <span>Voucher</span>
          <p>${order.voucherCode || "No voucher"}</p>
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
              <p><strong>${item.productName}</strong></p>
              <p class="payment-meta">${item.color} / Size ${item.size} / Qty ${item.quantity}</p>
            </div>
            <p>${window.storefrontState.formatMoney(item.lineTotalMinor)}</p>
          </article>
        `).join("")}
      </div>
    </article>
  `).join("");
})();
