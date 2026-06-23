(async function renderHomeCatalog() {
  const grid = document.getElementById("product-grid");
  if (!grid) {
    return;
  }

  function escapeHtml(value) {
    return String(value ?? "").replace(/[&<>"']/g, (character) => ({
      "&": "&amp;",
      "<": "&lt;",
      ">": "&gt;",
      "\"": "&quot;",
      "'": "&#39;"
    })[character]);
  }

  function productPrice(product) {
    return window.storefrontState.formatMoney(product.priceMinor || 0);
  }

  function productSummary(product) {
    const variantCount = (product.variants || []).filter((variant) => variant.active !== false).length;
    const variantLabel = variantCount === 1 ? "1 variant" : `${variantCount} variants`;
    return `${product.category || "Fashion"} / ${productPrice(product)} / ${variantLabel}`;
  }

  const catalog = await window.storefrontState.loadProductsWithApi();
  const products = catalog.products.length > 0 ? catalog.products : window.storefrontState.getProducts();

  if (products.length === 0) {
    grid.innerHTML = `
      <div class="empty-state">
        <h3>No products available</h3>
        <p>Catalog data could not be loaded from the backend or the local fallback.</p>
      </div>
    `;
    return;
  }

  grid.innerHTML = products.map((product) => {
    const image = product.images && product.images.length > 0 ? product.images[0] : "";
    return `
      <a class="product-card" href="product.html?id=${encodeURIComponent(product.productId)}">
        <img src="${escapeHtml(image)}" alt="${escapeHtml(product.name)}">
        <div>
          <h3>${escapeHtml(product.name)}</h3>
          <p>${escapeHtml(productSummary(product))}</p>
        </div>
      </a>
    `;
  }).join("");
})();
