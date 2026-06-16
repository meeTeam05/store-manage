(function renderAdminDashboard() {
  const session = window.storefrontState.getSession();
  const sessionElement = document.getElementById("admin-session");
  if (!session || String(session.role) !== "Admin") {
    window.location.href = "login.html";
    return;
  }

  if (sessionElement) {
    sessionElement.textContent = `Signed in as ${session.displayName || session.username} (${session.role}).`;
  }
})();
