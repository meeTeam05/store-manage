(function renderWishlistPage() {
  function isCustomerRole(role) {
    return role === "Customer" || role === 0;
  }

  const session = window.storefrontState.getSession();
  if (!session) {
    window.location.href = "login.html?returnTo=wishlist.html";
    return;
  }

  if (!isCustomerRole(session.role)) {
    window.location.href = "index.html";
    return;
  }

  const sessionElement = document.getElementById("wishlist-session");
  const statusElement = document.getElementById("wishlist-status");
  const emptyState = document.getElementById("wishlist-empty");
  const listElement = document.getElementById("wishlist-list");

  function preferredVariant(product) {
    return product.variants.find((entry) => entry.color === "Black" && entry.size === "M")
      || product.variants[0]
      || null;
  }

  function render() {
    const products = window.storefrontState.getWishlistProducts();
    if (sessionElement) {
      sessionElement.textContent = `Signed in as ${session.displayName || session.customerName || session.username}.`;
    }

    if (products.length === 0) {
      emptyState.hidden = false;
      listElement.innerHTML = "";
      return;
    }

    emptyState.hidden = true;
    listElement.innerHTML = products.map((product) => {
      const variant = preferredVariant(product);
      return `
        <article class="payment-card wishlist-card">
          <div class="wishlist-card-media">
            <img src="${product.images?.[0] || ""}" alt="${product.name}">
          </div>
          <div class="wishlist-card-copy">
            <p class="eyebrow">${product.category}</p>
            <h2>${product.name}</h2>
            <p class="payment-meta">${product.description}</p>
            <p class="price">${window.storefrontState.formatMoney(product.priceMinor)}</p>
            <p class="payment-meta">${variant ? `${variant.color} / Size ${variant.size}` : "No variant available"}</p>
            <div class="confirmation-actions">
              <button class="button primary compact" type="button" data-action="add" data-product-id="${product.productId}" data-variant-id="${variant ? variant.variantId : ""}">
                Add To Cart
              </button>
              <button class="button compact" type="button" data-action="remove" data-product-id="${product.productId}">
                Remove
              </button>
            </div>
          </div>
        </article>
      `;
    }).join("");

    listElement.querySelectorAll("[data-action='add']").forEach((button) => {
      button.addEventListener("click", async () => {
        const variantId = button.dataset.variantId;
        const productId = button.dataset.productId;
        if (!variantId) {
          statusElement.textContent = "This wishlist item has no purchasable variant.";
          statusElement.dataset.state = "error";
          return;
        }
        const result = await window.storefrontState.addToCartWithApi(productId, variantId, 1);
        statusElement.textContent = result.ok
          ? "Added wishlist item to cart."
          : result.error;
        statusElement.dataset.state = result.ok ? "success" : "error";
      });
    });

    listElement.querySelectorAll("[data-action='remove']").forEach((button) => {
      button.addEventListener("click", async () => {
        const result = await window.storefrontState.toggleWishlist(button.dataset.productId);
        statusElement.textContent = result.ok
          ? "Removed item from wishlist."
          : result.error;
        statusElement.dataset.state = result.ok ? "success" : "error";
        if (result.ok) {
          render();
        }
      });
    });
  }

  render();
})();
