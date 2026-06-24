const form = document.getElementById("login-form");
const statusElement = document.getElementById("status");
const roleHintsElement = document.getElementById("role-hints");
const registerForm = document.getElementById("register-form");
const registerStatusElement = document.getElementById("register-status");

if (roleHintsElement) {
  roleHintsElement.innerHTML = `
    <article class="role-card">
      <strong>Customer</strong>
      <span>Username: client001</span>
      <span>Password: 123456</span>
    </article>
    <article class="role-card">
      <strong>Staff</strong>
      <span>Username: staff001</span>
      <span>Password: staff123</span>
    </article>
    <article class="role-card">
      <strong>Admin</strong>
      <span>Username: admin001</span>
      <span>Password: admin123</span>
    </article>
  `;
}

function targetPathForRole(role) {
  if (role === "Admin" || role === 3) {
    return "admin.html";
  }
  if (role === "Manager" || role === 2) {
    return "admin.html";
  }
  if (role === "Staff" || role === 1) {
    return "staff.html";
  }
  return "cart.html";
}

function requestedReturnPath() {
  const params = new URLSearchParams(window.location.search);
  const returnTo = params.get("returnTo");
  const allowedTargets = new Set(["index.html", "product.html", "cart.html", "payment.html", "orders.html", "wishlist.html"]);
  return allowedTargets.has(String(returnTo)) ? String(returnTo) : null;
}

function isCustomerRole(role) {
  return role === "Customer" || role === 0;
}

if (form && statusElement) {
  form.addEventListener("submit", async (event) => {
    event.preventDefault();
    const formData = new FormData(form);
    const username = String(formData.get("username") || "").trim();
    const password = String(formData.get("password") || "").trim();
    const result = await window.storefrontState.signInWithApi(username, password);
    if (!result.ok) {
      statusElement.textContent = result.error;
      statusElement.dataset.state = "error";
      return;
    }

    const name = result.session.displayName || result.session.customerName || result.session.username;
    statusElement.textContent = `Signed in as ${name} (${result.session.role}).`;
    statusElement.dataset.state = "success";
    const returnTo = requestedReturnPath();
    if (returnTo && isCustomerRole(result.session.role)) {
      window.location.href = returnTo;
      return;
    }
    window.location.href = targetPathForRole(result.session.role);
  });
}

if (registerForm && registerStatusElement) {
  registerForm.addEventListener("submit", async (event) => {
    event.preventDefault();
    const formData = new FormData(registerForm);
    const result = await window.storefrontState.registerCustomer({
      fullName: String(formData.get("full_name") || "").trim(),
      phone: String(formData.get("phone") || "").trim(),
      city: String(formData.get("city") || "").trim(),
      line1: String(formData.get("line1") || "").trim(),
      username: String(formData.get("username") || "").trim(),
      password: String(formData.get("password") || "").trim()
    });
    if (!result.ok) {
      registerStatusElement.textContent = result.error;
      registerStatusElement.dataset.state = "error";
      return;
    }

    const name = result.session.displayName || result.session.customerName || result.session.username;
    registerStatusElement.textContent = `Registered and signed in as ${name}.`;
    registerStatusElement.dataset.state = "success";
    const returnTo = requestedReturnPath();
    window.location.href = returnTo || "cart.html";
  });
}
