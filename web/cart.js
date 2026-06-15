(function renderCartPage() {
  const cartItemsContainer = document.getElementById("cart-items-list");
  const subtotalElement = document.getElementById("summary-subtotal");
  const discountElement = document.getElementById("summary-discount");
  const totalElement = document.getElementById("summary-total");
  const checkoutButton = document.getElementById("checkout-link");
  const emptyState = document.getElementById("cart-empty-state");

  function render() {
    const summary = window.storefrontState.buildCartSummary();

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
      button.addEventListener("click", () => {
        const variantId = button.dataset.variantId;
        const item = summary.items.find((entry) => entry.variantId === variantId);
        if (!item) {
          return;
        }

        if (button.dataset.action === "increase") {
          window.storefrontState.setCartQuantity(variantId, item.quantity + 1);
        } else if (button.dataset.action === "decrease") {
          window.storefrontState.setCartQuantity(variantId, item.quantity - 1);
        } else {
          window.storefrontState.removeFromCart(variantId);
        }
        render();
      });
    });

    const session = window.storefrontState.getSession();
    checkoutButton.textContent = session ? "Proceed To Checkout" : "Sign In To Checkout";
    checkoutButton.href = session ? "login.html#checkout-ready" : "login.html";
  }

  render();
})();
