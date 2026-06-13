const form = document.getElementById("login-form");
const statusElement = document.getElementById("status");

if (form && statusElement) {
  form.addEventListener("submit", (event) => {
    event.preventDefault();
    const formData = new FormData(form);
    const username = String(formData.get("username") || "").trim();

    statusElement.textContent = username
      ? `Static luxury shell only. Connect this form to application auth later. Submitted for ${username}.`
      : "Username is required.";
  });
}
