const form = document.getElementById("login-form");
const statusElement = document.getElementById("status");
const roleHintsElement = document.getElementById("role-hints");

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
  if (role === "Staff" || role === 1 || role === "Manager" || role === 2) {
    return "staff.html";
  }
  return "cart.html";
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
    window.location.href = targetPathForRole(result.session.role);
  });
}
