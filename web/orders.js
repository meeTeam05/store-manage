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
  const notificationsShell = document.getElementById("orders-notifications-shell");
  const notificationsList = document.getElementById("orders-notifications-list");
  const ordersList = document.getElementById("orders-list");
  let ordersCache = [];
  let ordersSource = "local";
  let notificationsCache = [];
  let reviewsCache = [];
  const returnsByOrder = new Map();

  if (sessionElement) {
    sessionElement.textContent = `Signed in as ${session.displayName || session.customerName || session.username}.`;
  }

  function canCancel(order) {
    return window.storefrontState.canCancelOrder
      ? window.storefrontState.canCancelOrder(order)
      : ["AwaitingPayment", "Paid"].includes(order.orderStatus);
  }

  function reviewForItem(order, item) {
    return reviewsCache.find((entry) =>
      entry.orderId === order.orderId &&
      entry.productId === item.productId &&
      entry.variantId === item.variantId
    ) || null;
  }

  function returnRecordsForItem(order, item) {
    return (returnsByOrder.get(order.orderId) || []).filter((entry) => entry.orderItemId === item.orderItemId);
  }

  function renderItemActions(order, item) {
    const review = reviewForItem(order, item);
    const returnRecords = returnRecordsForItem(order, item);
    const canReview = window.storefrontState.createReview && ["Delivered", "Completed"].includes(order.orderStatus);
    const canReturn = window.storefrontState.createReturnRequest && ["Delivered", "Completed"].includes(order.orderStatus);
    const activeReturn = returnRecords.find((entry) => !["Rejected", "Closed"].includes(entry.status)) || null;

    return `
      <div class="order-item-actions">
        <div class="order-item-panel">
          <p class="summary-card-title">Review</p>
          ${review
            ? `
              <div class="order-action-card">
                <p><strong>${escapeHtml(review.rating)}/5</strong></p>
                <p class="payment-meta">${escapeHtml(review.comment || "No comment")}</p>
              </div>
            `
            : canReview
              ? `
                <form class="order-inline-form" data-review-form data-order-id="${escapeHtml(order.orderId)}" data-product-id="${escapeHtml(item.productId)}" data-variant-id="${escapeHtml(item.variantId)}">
                  <label class="admin-field">
                    <span>Rating</span>
                    <select name="rating">
                      <option value="5">5</option>
                      <option value="4">4</option>
                      <option value="3">3</option>
                      <option value="2">2</option>
                      <option value="1">1</option>
                    </select>
                  </label>
                  <label class="admin-field admin-field-full">
                    <span>Comment</span>
                    <textarea name="comment" rows="3" placeholder="Share quick feedback"></textarea>
                  </label>
                  <div class="admin-actions">
                    <button class="button compact" type="submit">Submit Review</button>
                  </div>
                </form>
              `
              : '<p class="payment-meta">Available after delivery.</p>'}
        </div>

        <div class="order-item-panel">
          <p class="summary-card-title">Return Request</p>
          ${returnRecords.length > 0
            ? `
              <div class="order-action-stack">
                ${returnRecords.map((entry) => `
                  <article class="order-action-card">
                    <div class="order-history-status">
                      <span>${escapeHtml(entry.status)}</span>
                    </div>
                    <p class="payment-meta">Qty ${escapeHtml(entry.quantity)} / ${escapeHtml(entry.reason)}</p>
                  </article>
                `).join("")}
              </div>
            `
            : ""}
          ${canReturn && !activeReturn
            ? `
              <form class="order-inline-form" data-return-form data-order-id="${escapeHtml(order.orderId)}" data-order-item-id="${escapeHtml(item.orderItemId)}" data-max-quantity="${escapeHtml(item.quantity)}">
                <label class="admin-field">
                  <span>Quantity</span>
                  <input name="quantity" type="number" min="1" max="${escapeHtml(item.quantity)}" step="1" value="1">
                </label>
                <label class="admin-field admin-field-full">
                  <span>Reason</span>
                  <textarea name="reason" rows="3" placeholder="Reason for return"></textarea>
                </label>
                <div class="admin-actions">
                  <button class="button compact" type="submit">Request Return</button>
                </div>
              </form>
            `
            : (activeReturn ? '<p class="payment-meta">There is already an active return request for this item.</p>' : '<p class="payment-meta">Available after delivery.</p>')}
        </div>
      </div>
    `;
  }

  function renderNotifications() {
    if (!notificationsShell || !notificationsList) {
      return;
    }
    if (notificationsCache.length === 0) {
      notificationsShell.hidden = true;
      notificationsList.innerHTML = "";
      return;
    }
    notificationsShell.hidden = false;
    notificationsList.innerHTML = notificationsCache.map((notification) => `
      <article class="payment-item order-history-item-rich">
        <div class="order-item-main">
          <div>
            <p><strong>${escapeHtml(notification.title)}</strong></p>
            <p class="payment-meta">${escapeHtml(new Date(notification.createdAtIso).toLocaleString("vi-VN"))}</p>
          </div>
          <div class="order-history-status">
            <span>${escapeHtml(notification.returnStatus || "Updated")}</span>
            <span>${notification.isRead ? "Read" : "Unread"}</span>
          </div>
        </div>
        <p class="payment-meta">${escapeHtml(notification.message)}</p>
        ${notification.extraDetail ? `<p class="payment-meta">${escapeHtml(notification.extraDetail)}</p>` : ""}
        ${notification.isRead ? "" : `<div class="staff-order-actions"><button class="button compact" type="button" data-mark-notification-read="${escapeHtml(notification.notificationId)}">Mark Read</button></div>`}
      </article>
    `).join("");

    notificationsList.querySelectorAll("[data-mark-notification-read]").forEach((button) => {
      button.addEventListener("click", async () => {
        const notificationId = String(button.dataset.markNotificationRead || "");
        button.disabled = true;
        const result = window.storefrontState.markNotificationReadWithApi
          ? await window.storefrontState.markNotificationReadWithApi(notificationId)
          : window.storefrontState.markNotificationRead(notificationId);
        feedbackElement.dataset.state = result.ok ? "success" : "error";
        feedbackElement.textContent = result.ok
          ? "Notification marked as read."
          : result.error;
        await render();
      });
    });
  }

  async function renderOrders(orders) {
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
            <article class="payment-item order-history-item-rich">
              <div class="order-item-main">
                <div>
                  <p><strong>${escapeHtml(item.productName)}</strong></p>
                  <p class="payment-meta">${escapeHtml(item.color)} / Size ${escapeHtml(item.size)} / Qty ${escapeHtml(item.quantity)}</p>
                </div>
                <p>${window.storefrontState.formatMoney(item.lineTotalMinor)}</p>
              </div>
              ${renderItemActions(order, item)}
            </article>
          `).join("")}
        </div>

        <div class="staff-order-actions">
          ${canCancel(order)
            ? `<button class="button compact" type="button" data-cancel-order="${escapeHtml(order.orderId)}">Cancel Order</button>`
            : '<p class="payment-meta staff-order-note">This order can no longer be cancelled.</p>'}
        </div>
      </article>
    `).join("");

    ordersList.querySelectorAll("[data-cancel-order]").forEach((button) => {
      button.addEventListener("click", async () => {
        const orderId = String(button.dataset.cancelOrder || "");
        button.disabled = true;
        const result = window.storefrontState.cancelCustomerOrderWithApi
          ? await window.storefrontState.cancelCustomerOrderWithApi(orderId)
          : window.storefrontState.cancelCustomerOrder(orderId);
        feedbackElement.dataset.state = result.ok ? "success" : "error";
        feedbackElement.textContent = result.ok
          ? `${result.order.orderNumber} cancelled successfully.`
          : result.error;
        await render();
      });
    });

    ordersList.querySelectorAll("[data-review-form]").forEach((form) => {
      form.addEventListener("submit", async (event) => {
        event.preventDefault();
        const formData = new FormData(form);
        const submitButton = form.querySelector('button[type="submit"]');
        submitButton.disabled = true;
        const result = await window.storefrontState.createReview({
          orderId: String(form.dataset.orderId || ""),
          productId: String(form.dataset.productId || ""),
          variantId: String(form.dataset.variantId || ""),
          rating: Number(formData.get("rating") || 5),
          comment: String(formData.get("comment") || "").trim()
        });
        feedbackElement.dataset.state = result.ok ? "success" : "error";
        feedbackElement.textContent = result.ok
          ? "Review submitted successfully."
          : result.error;
        await render();
      });
    });

    ordersList.querySelectorAll("[data-return-form]").forEach((form) => {
      form.addEventListener("submit", async (event) => {
        event.preventDefault();
        const formData = new FormData(form);
        const submitButton = form.querySelector('button[type="submit"]');
        submitButton.disabled = true;
        const result = await window.storefrontState.createReturnRequest({
          orderId: String(form.dataset.orderId || ""),
          orderItemId: String(form.dataset.orderItemId || ""),
          quantity: Number(formData.get("quantity") || 1),
          reason: String(formData.get("reason") || "").trim()
        });
        feedbackElement.dataset.state = result.ok ? "success" : "error";
        feedbackElement.textContent = result.ok
          ? "Return request created successfully."
          : result.error;
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

  async function loadOrderRelatedState() {
    returnsByOrder.clear();
    if (window.storefrontState.loadCustomerNotificationsWithApi) {
      const result = await window.storefrontState.loadCustomerNotificationsWithApi();
      notificationsCache = result.notifications || [];
    } else {
      notificationsCache = window.storefrontState.getCustomerNotifications ? window.storefrontState.getCustomerNotifications() : [];
    }
    if (window.storefrontState.loadCustomerReviewsWithApi) {
      const result = await window.storefrontState.loadCustomerReviewsWithApi();
      reviewsCache = result.reviews;
    } else {
      reviewsCache = window.storefrontState.getCustomerReviews ? window.storefrontState.getCustomerReviews() : [];
    }

    await Promise.all(ordersCache.map(async (order) => {
      if (window.storefrontState.loadOrderReturnsWithApi) {
        const result = await window.storefrontState.loadOrderReturnsWithApi(order.orderId);
        returnsByOrder.set(order.orderId, result.returns || []);
      } else {
        returnsByOrder.set(order.orderId, window.storefrontState.getOrderReturns ? window.storefrontState.getOrderReturns(order.orderId) : []);
      }
    }));
  }

  async function render() {
    await loadOrders();
    await loadOrderRelatedState();
    renderNotifications();
    await renderOrders(ordersCache);
    if (feedbackElement && !feedbackElement.textContent) {
      feedbackElement.dataset.state = "success";
      feedbackElement.textContent = ordersSource === "api"
        ? "Loaded your current backend orders."
        : "Showing local session orders.";
    }
  }

  await render();
})();
