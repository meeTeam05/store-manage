(function renderStaffDashboard() {
  const session = window.storefrontState.getSession();
  const sessionElement = document.getElementById("staff-session");
  if (!session || !["Staff", "Manager", "Admin"].includes(String(session.role))) {
    window.location.href = "login.html";
    return;
  }

  if (sessionElement) {
    sessionElement.textContent = `Signed in as ${session.displayName || session.username} (${session.role}).`;
  }
})();
