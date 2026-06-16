(function renderProductPage() {
  const product = window.storefrontState.getProduct("product-001");
  if (!product) {
    return;
  }

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

  let selectedSize = "M";
  let selectedColor = "Black";

  title.textContent = product.name;
  category.textContent = product.category;
  price.textContent = window.storefrontState.formatMoney(product.priceMinor);
  description.textContent = product.description;

  gallery.innerHTML = product.images.map((src, index) => `
    <img src="${src}" alt="${product.name} view ${index + 1}">
  `).join("");

  function renderChips(row, values, selected, onSelect) {
    row.innerHTML = values.map((value) => `
      <button class="chip ${value === selected ? "is-active" : ""}" type="button" data-value="${value}">${value}</button>
    `).join("");
    row.querySelectorAll(".chip").forEach((button) => {
      button.addEventListener("click", () => onSelect(button.dataset.value));
    });
  }

  function findVariant() {
    return product.variants.find((entry) => entry.size === selectedSize && entry.color === selectedColor) || null;
  }

  function renderOptions() {
    const sizes = [...new Set(product.variants.map((entry) => entry.size))];
    const colors = [...new Set(product.variants.map((entry) => entry.color))];
    renderChips(sizeRow, sizes, selectedSize, (value) => {
      selectedSize = value;
      renderOptions();
    });
    renderChips(colorRow, colors, selectedColor, (value) => {
      selectedColor = value;
      renderOptions();
    });
  }

  renderOptions();

  addToCartButton.addEventListener("click", async () => {
    const variant = findVariant();
    if (!variant) {
      status.textContent = "Selected variant is unavailable.";
      return;
    }
    const result = await window.storefrontState.addToCartWithApi(product.productId, variant.variantId, 1);
    status.textContent = result.ok
      ? `Added ${product.name} / ${variant.color} / ${variant.size} to cart.`
      : result.error;
  });

  wishlistButton.addEventListener("click", (event) => {
    event.preventDefault();
    const session = window.storefrontState.getSession();
    status.textContent = session
      ? `Saved to wishlist for ${session.customerName} in demo mode.`
      : "Sign in first to use wishlist.";
  });
})();
