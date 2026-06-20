(function renderPaymentPage() {
  const session = window.storefrontState.getSession();
  if (!session) {
    window.location.href = "login.html";
    return;
  }

  const summary = window.storefrontState.buildCartSummary();
  if (summary.items.length === 0) {
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

  infoContainer.innerHTML = `
    <article>
      <span>Customer</span>
      <p>${session.displayName || session.customerName || session.username}</p>
    </article>
    <article>
      <span>Delivery</span>
      <p>${customer ? `${customer.phone}, ${customer.city}` : "Local test profile"}</p>
    </article>
  `;

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

  form.addEventListener("submit", async (event) => {
    event.preventDefault();
    const formData = new FormData(form);
    const paymentMethod = String(formData.get("payment-method") || "Cash");
    const response = window.storefrontApi
      ? await window.storefrontApi.checkout({ method: paymentMethod })
      : { ok: false, error: "API unavailable" };

    window.storefrontState.clearCart();
    statusElement.dataset.state = "success";
    statusElement.textContent = response.ok
      ? `Order placed with ${paymentMethod}.`
      : `Local order placed with ${paymentMethod}. Backend checkout is not connected yet.`;
    form.querySelector("button[type='submit']").disabled = true;
    form.querySelector("button[type='submit']").textContent = "Order Submitted";
  });
})();
