(async function renderProductPage() {
  const gallery = document.getElementById("product-gallery");
  const title = document.getElementById("product-title");
  const category = document.getElementById("product-category");
  const price = document.getElementById("product-price");
  const description = document.getElementById("product-description");
  const sizeRow = document.getElementById("size-row");
  const colorRow = document.getElementById("color-row");
  const addToCartButton = document.getElementById("add-to-cart");
  const wishlistButton = document.getElementById("wishlist-link");
  const status = document.getElementById("product-status");

  function escapeHtml(value) {
    return String(value ?? "").replace(/[&<>"']/g, (character) => ({
      "&": "&amp;",
      "<": "&lt;",
      ">": "&gt;",
      "\"": "&quot;",
      "'": "&#39;"
    })[character]);
  }

  const catalog = await window.storefrontState.loadProductsWithApi();
  const products = catalog.products || window.storefrontState.getProducts();
  const requestedId = new URLSearchParams(window.location.search).get("id");
  const product = products.find((entry) => entry.productId === requestedId) || products[0] || null;
  if (!product) {
    title.textContent = "Product unavailable";
    status.textContent = "No product could be loaded from the catalog.";
    addToCartButton.disabled = true;
    wishlistButton.hidden = true;
    return;
  }

  const variants = product.variants.filter((entry) => entry.active !== false);
  let selectedVariant = variants[0] || null;

  title.textContent = product.name;
  category.textContent = product.category;
  description.textContent = product.description;
  gallery.innerHTML = product.images.map((src, index) => `
    <img src="${escapeHtml(src)}" alt="${escapeHtml(product.name)} view ${index + 1}">
  `).join("");

  function renderChips(row, values, selected, disabledValues = [], onSelect) {
    row.innerHTML = values.map((value) => {
      const isDisabled = disabledValues.includes(value);
      return `
        <button class="chip ${value === selected ? "is-active" : ""} ${isDisabled ? "is-unavailable" : ""}" type="button" data-value="${escapeHtml(value)}">
          ${escapeHtml(value)}
        </button>
      `;
    }).join("");
    row.querySelectorAll("[data-value]").forEach((button) => {
      button.addEventListener("click", () => onSelect(String(button.dataset.value || "")));
    });
  }

  function renderOptions() {
    if (!selectedVariant) {
      price.textContent = window.storefrontState.formatMoney(product.priceMinor);
      sizeRow.innerHTML = "";
      colorRow.innerHTML = "";
      addToCartButton.disabled = true;
      status.textContent = "This product has no active variants.";
      return;
    }
    const sizes = [...new Set(variants.map((entry) => entry.size))];
    const colors = [...new Set(variants.map((entry) => entry.color))];

    const disabledSizes = sizes.filter((size) => !variants.some((entry) => entry.size === size && entry.color === selectedVariant.color));
    const disabledColors = colors.filter((color) => !variants.some((entry) => entry.color === color && entry.size === selectedVariant.size));

    renderChips(sizeRow, sizes, selectedVariant.size, disabledSizes, (size) => {
      selectedVariant = variants.find((entry) => entry.size === size && entry.color === selectedVariant.color)
        || variants.find((entry) => entry.size === size)
        || selectedVariant;
      renderOptions();
    });
    renderChips(colorRow, colors, selectedVariant.color, disabledColors, (color) => {
      selectedVariant = variants.find((entry) => entry.size === selectedVariant.size && entry.color === color)
        || variants.find((entry) => entry.color === color)
        || selectedVariant;
      renderOptions();
    });
    price.textContent = window.storefrontState.formatMoney(selectedVariant.priceMinor || product.priceMinor);
    addToCartButton.disabled = selectedVariant.stockQuantity <= 0;
    status.textContent = selectedVariant.stockQuantity > 0
      ? `${selectedVariant.stockQuantity} item(s) available.`
      : "Selected variant is out of stock.";
  }

  function renderWishlistButton() {
    const session = window.storefrontState.getSession();
    if (!session || !session.customerId) {
      wishlistButton.textContent = "Sign In To Save";
      wishlistButton.href = `login.html?returnTo=${encodeURIComponent(`product.html?id=${product.productId}`)}`;
      return;
    }
    const saved = window.storefrontState.isInWishlist(product.productId);
    wishlistButton.textContent = saved ? "Remove From Wishlist" : "Save To Wishlist";
    wishlistButton.href = "wishlist.html";
  }

  addToCartButton.addEventListener("click", async () => {
    if (!selectedVariant) return;
    addToCartButton.disabled = true;
    const result = await window.storefrontState.addToCartWithApi(product.productId, selectedVariant.variantId, 1);
    status.textContent = result.ok
      ? `Added ${product.name} / ${selectedVariant.color} / ${selectedVariant.size} to cart.`
      : result.error;
    addToCartButton.disabled = selectedVariant.stockQuantity <= 0;
  });

  wishlistButton.addEventListener("click", async (event) => {
    const session = window.storefrontState.getSession();
    if (!session || !session.customerId) return;
    event.preventDefault();
    const result = await window.storefrontState.toggleWishlist(product.productId);
    status.textContent = result.ok
      ? (result.saved ? `Saved ${product.name} to wishlist.` : `Removed ${product.name} from wishlist.`)
      : result.error;
    if (result.ok) renderWishlistButton();
  });

  renderOptions();
  renderWishlistButton();
})();
