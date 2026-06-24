(async function renderHomeCatalog() {
  const grid = document.getElementById("product-grid");
  const prevBtn = document.getElementById("slider-prev");
  const nextBtn = document.getElementById("slider-next");
  const dotsContainer = document.getElementById("slider-dots");
  
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
  const products = catalog.products && catalog.products.length > 0 ? catalog.products : window.storefrontState.getProducts();

  if (products.length === 0) {
    grid.innerHTML = `
      <div class="empty-state">
        <h3>No products available</h3>
        <p>Catalog data could not be loaded from the backend or the local fallback.</p>
      </div>
    `;
    return;
  }

  // Render cards
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

  // Setup dots
  if (dotsContainer) {
    dotsContainer.innerHTML = products.map((_, i) => `
      <button class="dot ${i === 0 ? "is-active" : ""}" data-index="${i}" type="button" aria-label="Go to slide ${i+1}"></button>
    `).join("");

    const dots = dotsContainer.querySelectorAll(".dot");
    dots.forEach((dot) => {
      dot.addEventListener("click", () => {
        const index = parseInt(dot.dataset.index || "0", 10);
        const cardWidth = grid.querySelector(".product-card")?.getBoundingClientRect().width || 0;
        const gap = 20; // grid gap
        grid.scrollTo({
          left: index * (cardWidth + gap),
          behavior: "smooth"
        });
      });
    });

    // Update active dot on scroll
    grid.addEventListener("scroll", () => {
      const cardWidth = grid.querySelector(".product-card")?.getBoundingClientRect().width || 0;
      const gap = 20;
      const scrollPos = grid.scrollLeft;
      const activeIndex = Math.round(scrollPos / (cardWidth + gap));
      
      dots.forEach((dot, idx) => {
        if (idx === activeIndex) {
          dot.classList.add("is-active");
        } else {
          dot.classList.remove("is-active");
        }
      });
    });
  }

  // Setup arrows
  if (prevBtn) {
    prevBtn.addEventListener("click", () => {
      const cardWidth = grid.querySelector(".product-card")?.getBoundingClientRect().width || 0;
      const gap = 20;
      grid.scrollBy({
        left: -(cardWidth + gap),
        behavior: "smooth"
      });
    });
  }

  if (nextBtn) {
    nextBtn.addEventListener("click", () => {
      const cardWidth = grid.querySelector(".product-card")?.getBoundingClientRect().width || 0;
      const gap = 20;
      grid.scrollBy({
        left: (cardWidth + gap),
        behavior: "smooth"
      });
    });
  }
})();
