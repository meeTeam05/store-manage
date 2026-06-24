(function renderPaymentPage() {
  function isCustomerRole(role) {
    return role === "Customer" || role === 0;
  }

  const session = window.storefrontState.getSession();
  if (!session) {
    window.location.href = "login.html?returnTo=payment.html";
    return;
  }

  if (!isCustomerRole(session.role)) {
    window.location.href = "login.html?returnTo=payment.html";
    return;
  }

  const summary = window.storefrontState.buildCartSummary();
  const existingOrders = window.storefrontState.getCustomerOrders();
  if (summary.items.length === 0 && existingOrders.length === 0) {
    window.location.href = "cart.html";
    return;
  }

  const customer = window.storefrontState.getCustomerProfile();
  const infoContainer = document.getElementById("payment-info");
  const itemsContainer = document.getElementById("payment-items");
  const subtotalElement = document.getElementById("payment-subtotal");
  const discountElement = document.getElementById("payment-discount");
  const totalElement = document.getElementById("payment-total");
  const form = document.getElementById("payment-form");
  const statusElement = document.getElementById("payment-status");
  const confirmationBlock = document.getElementById("payment-confirmation");
  const confirmationInfo = document.getElementById("confirmation-info");
  const confirmationItems = document.getElementById("confirmation-items");
  const recentOrdersShell = document.getElementById("recent-orders-shell");
  const recentOrders = document.getElementById("recent-orders");
  const bankQrBlock = document.getElementById("bank-transfer-qr");
  const bankQrNote = document.getElementById("bank-transfer-note");
  const confirmationQrBlock = document.getElementById("bank-transfer-confirmation-qr");
  const confirmationQrNote = document.getElementById("bank-transfer-confirmation-note");
  function renderRecentOrders() {
    const orders = window.storefrontState.getCustomerOrders().slice(0, 3);
    if (orders.length === 0) {
      recentOrdersShell.hidden = true;
      return;
    }

    recentOrdersShell.hidden = false;
    recentOrders.innerHTML = orders.map((order) => `
      <article class="recent-order-card">
        <p><strong>${order.orderNumber}</strong></p>
        <p class="payment-meta">${order.paymentMethod} / ${order.orderStatus} / ${order.shippingStatus}</p>
        <p class="payment-meta">${window.storefrontState.formatMoney(order.totalMinor)}</p>
      </article>
    `).join("");
  }

  function renderConfirmation(order) {
    const isBankTransfer = order.paymentMethod === "BankTransfer";
    const transferReference = order.paymentReference || `BANK-${order.orderNumber}`;
    const referenceBlock = order.paymentReference ? `
      <article>
        <span>Reference</span>
        <p>${order.paymentReference}</p>
      </article>
    ` : "";
    confirmationBlock.hidden = false;
    if (confirmationQrBlock) {
      confirmationQrBlock.hidden = !isBankTransfer;
    }

    if (confirmationQrNote) {
      confirmationQrNote.textContent = transferReference;
    }
    confirmationInfo.innerHTML = `
      <article>
        <span>Order</span>
        <p>${order.orderNumber}</p>
      </article>
      <article>
        <span>Tracking</span>
        <p>${order.trackingCode}</p>
      </article>
      <article>
        <span>Payment</span>
        <p>${order.paymentMethod} / ${order.paymentStatus}</p>
      </article>
      ${referenceBlock}
    `;

    confirmationItems.innerHTML = order.items.map((item) => `
      <article class="payment-item">
        <div>
          <p><strong>${item.productName}</strong></p>
          <p class="payment-meta">${item.color} / Size ${item.size} / Qty ${item.quantity}</p>
        </div>
        <p>${window.storefrontState.formatMoney(item.lineTotalMinor)}</p>
      </article>
    `).join("");
  }
  function getSelectedPaymentMethod() {
    return String(
      document.querySelector('input[name="payment-method"]:checked')?.value || "Cash"
    );
  }

  function updateBankTransferQr(referenceText = "BANK-TRANSFER") {
    const selectedMethod = getSelectedPaymentMethod();
    const shouldShowQr = selectedMethod === "BankTransfer";

    if (bankQrBlock) {
      bankQrBlock.hidden = !shouldShowQr;
    }

    if (bankQrNote) {
      bankQrNote.textContent = referenceText;
    }
  }

  function setupBankTransferQr() {
    const paymentMethods = document.querySelectorAll('input[name="payment-method"]');

    paymentMethods.forEach((input) => {
      input.addEventListener("change", () => {
        updateBankTransferQr();
      });
    });

    updateBankTransferQr();
  }
  infoContainer.innerHTML = `
    <article>
      <span>Customer</span>
      <p>${session.displayName || session.customerName || session.username}</p>
    </article>
    <article>
      <span>Delivery</span>
      <p>${customer ? `${customer.phone}, ${customer.city}` : "Profile not available"}</p>
    </article>
  `;

  if (summary.items.length === 0) {
    form.hidden = true;
    itemsContainer.innerHTML = `
      <article class="recent-order-card">
        <p><strong>No active cart items.</strong></p>
        <p class="payment-meta">Your recent orders are shown below. Add a product to create another order.</p>
      </article>
    `;
    subtotalElement.textContent = window.storefrontState.formatMoney(0);
    discountElement.textContent = `- ${window.storefrontState.formatMoney(0)}`;
    totalElement.textContent = window.storefrontState.formatMoney(0);
    renderRecentOrders();
    return;
  }

  itemsContainer.innerHTML = summary.items.map((item) => `
    <article class="payment-item">
      <div>
        <p><strong>${item.productName}</strong></p>
        <p class="payment-meta">${item.color} / Size ${item.size} / Qty ${item.quantity}</p>
      </div>
      <p>${window.storefrontState.formatMoney(item.lineTotalMinor)}</p>
    </article>
  `).join("");

  subtotalElement.textContent = window.storefrontState.formatMoney(summary.subtotalMinor);
  discountElement.textContent = `- ${window.storefrontState.formatMoney(summary.discountMinor)}`;
  totalElement.textContent = window.storefrontState.formatMoney(summary.totalMinor);
  renderRecentOrders();

  setupBankTransferQr();
  form.addEventListener("submit", async (event) => {
    event.preventDefault();
    const formData = new FormData(form);
    const paymentMethod = String(formData.get("payment-method") || "Cash");
    const submitButton = form.querySelector("button[type='submit']");
    submitButton.disabled = true;
    submitButton.textContent = "Submitting...";

    const result = await window.storefrontState.placeOrder(paymentMethod);
    if (!result.ok) {
      statusElement.dataset.state = "error";
      statusElement.textContent = result.error;
      submitButton.disabled = false;
      submitButton.textContent = "Place Order";
      return;
    }

    statusElement.dataset.state = "success";
    statusElement.textContent = `${result.order.orderNumber} created with ${paymentMethod}.`;
    submitButton.textContent = "Order Submitted";
    renderConfirmation(result.order);
    renderRecentOrders();
    itemsContainer.innerHTML = "";
    subtotalElement.textContent = window.storefrontState.formatMoney(0);
    discountElement.textContent = `- ${window.storefrontState.formatMoney(0)}`;
    totalElement.textContent = window.storefrontState.formatMoney(0);
  });
})();
