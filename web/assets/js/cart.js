(function renderCartPage() {
  function isCustomerRole(role) {
    return role === "Customer" || role === 0;
  }

  const cartItemsContainer = document.getElementById("cart-items-list");
  const subtotalElement = document.getElementById("summary-subtotal");
  const discountElement = document.getElementById("summary-discount");
  const totalElement = document.getElementById("summary-total");
  const checkoutButton = document.getElementById("checkout-link");
  const emptyState = document.getElementById("cart-empty-state");

  function render() {
    const summary = window.storefrontState.buildCartSummary();
    const session = window.storefrontState.getSession();

    if (summary.items.length === 0) {
      cartItemsContainer.innerHTML = "";
      emptyState.hidden = false;
    } else {
      emptyState.hidden = true;
      cartItemsContainer.innerHTML = summary.items.map((item) => `
        <article class="cart-item">
          <img src="${item.image}" alt="${item.productName}">
          <div class="cart-copy">
            <h2>${item.productName}</h2>
            <p>${item.color} / Size ${item.size}</p>
            <div class="cart-controls">
              <button class="chip" type="button" data-action="decrease" data-variant-id="${item.variantId}">-</button>
              <span>Qty ${item.quantity}</span>
              <button class="chip" type="button" data-action="increase" data-variant-id="${item.variantId}">+</button>
              <button class="text-link" type="button" data-action="remove" data-variant-id="${item.variantId}">Remove</button>
            </div>
          </div>
          <p class="price">${window.storefrontState.formatMoney(item.lineTotalMinor)}</p>
        </article>
      `).join("");
    }

    subtotalElement.textContent = window.storefrontState.formatMoney(summary.subtotalMinor);
    discountElement.textContent = `- ${window.storefrontState.formatMoney(summary.discountMinor)}`;
    totalElement.textContent = window.storefrontState.formatMoney(summary.totalMinor);

    cartItemsContainer.querySelectorAll("[data-action]").forEach((button) => {
      button.addEventListener("click", async () => {
        const variantId = button.dataset.variantId;
        const item = summary.items.find((entry) => entry.variantId === variantId);
        if (!item) {
          return;
        }

        button.disabled = true;
        if (button.dataset.action === "increase") {
          if (window.storefrontState.setCartQuantityWithApi) {
            await window.storefrontState.setCartQuantityWithApi(variantId, item.quantity + 1);
          } else {
            window.storefrontState.setCartQuantity(variantId, item.quantity + 1);
          }
        } else if (button.dataset.action === "decrease") {
          if (window.storefrontState.setCartQuantityWithApi) {
            await window.storefrontState.setCartQuantityWithApi(variantId, item.quantity - 1);
          } else {
            window.storefrontState.setCartQuantity(variantId, item.quantity - 1);
          }
        } else {
          if (window.storefrontState.removeFromCartWithApi) {
            await window.storefrontState.removeFromCartWithApi(variantId);
          } else {
            window.storefrontState.removeFromCart(variantId);
          }
        }
        render();
      });
    });

    if (summary.items.length === 0) {
      checkoutButton.textContent = "Add Product First";
      checkoutButton.href = "product.html";
      return;
    }

    if (!session) {
      checkoutButton.textContent = "Sign In To Continue";
      checkoutButton.href = "login.html?returnTo=payment.html";
      return;
    }

    if (!isCustomerRole(session.role)) {
      checkoutButton.textContent = "Use Customer Account";
      checkoutButton.href = "login.html?returnTo=payment.html";
      return;
    }

    checkoutButton.textContent = "Continue To Payment";
    checkoutButton.href = "payment.html";
  }

  render();
})();
