(function attachSessionShell() {
  const nav = document.querySelector(".nav");
  if (!nav || !window.storefrontState) {
    return;
  }

  const authLinks = Array.from(nav.querySelectorAll("[data-auth-link]"));
  const session = window.storefrontState.getSession();

  authLinks.forEach((link) => {
    link.textContent = session ? "Switch Account" : "Sign In";
    link.href = "login.html";
  });

  const previous = document.getElementById("nav-session-shell");
  if (previous) {
    previous.remove();
  }

  if (!session) {
    return;
  }

  const role = String(session.role);
  const currentPage = window.location.pathname.split("/").pop() || "index.html";
  const showOrdersLink = (role === "Customer" || role === "0") && currentPage !== "orders.html";
  const workspaceLink = (role === "Admin" || role === "3")
    ? "admin.html"
    : (role === "Staff" || role === "Manager" || role === "1" || role === "2" ? "staff.html" : null);

  const shell = document.createElement("div");
  shell.id = "nav-session-shell";
  shell.className = "nav-session";
  shell.innerHTML = `
    ${showOrdersLink ? '<a class="nav-aux" href="orders.html">My Orders</a>' : ""}
    ${workspaceLink && workspaceLink !== currentPage ? `<a class="nav-aux" href="${workspaceLink}">Workspace</a>` : ""}
    <span class="nav-user">${session.displayName || session.customerName || session.username} / ${role}</span>
    <button id="nav-signout" class="nav-logout" type="button">Sign Out</button>
  `;
  nav.appendChild(shell);

  const signOutButton = document.getElementById("nav-signout");
  if (signOutButton) {
    signOutButton.addEventListener("click", () => {
      window.storefrontState.signOut();
      window.location.href = "index.html";
    });
  }
})();
