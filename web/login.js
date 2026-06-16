const form = document.getElementById("login-form");
const statusElement = document.getElementById("status");
const roleHintsElement = document.getElementById("role-hints");

if (roleHintsElement) {
  roleHintsElement.innerHTML = `
    <span>Customer: client001 / 123456</span>
    <span>Staff: staff001 / staff123</span>
    <span>Admin: admin001 / admin123</span>
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
      return;
    }

    const name = result.session.displayName || result.session.customerName || result.session.username;
    statusElement.textContent = `Signed in as ${name} (${result.session.role}).`;
    window.location.href = targetPathForRole(result.session.role);
  });
}
