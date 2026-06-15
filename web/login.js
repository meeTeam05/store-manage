const form = document.getElementById("login-form");
const statusElement = document.getElementById("status");

if (form && statusElement) {
  form.addEventListener("submit", (event) => {
    event.preventDefault();
    const formData = new FormData(form);
    const username = String(formData.get("username") || "").trim();
    const password = String(formData.get("password") || "").trim();
    const result = window.storefrontState.signIn(username, password);
    statusElement.textContent = result.ok
      ? `Signed in as ${result.session.customerName}. Cart and wishlist demo state are now linked.`
      : result.error;
  });
}
