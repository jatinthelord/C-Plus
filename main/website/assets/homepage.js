"use strict";

(() => {
  const data = window.CSP_RELEASE_DATA || { current: "", packages: [] };
  const query = selector => document.querySelector(selector);
  const escape = value => String(value).replace(/[&<>"']/g, character => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  }[character]));

  document.querySelectorAll("[data-current-version]").forEach(element => {
    element.textContent = data.current;
  });

  const form = query(".home-columns form");
  const input = query("#home-q");
  if (!form || !input || !data.packages.length) return;

  const suggestions = document.createElement("div");
  suggestions.className = "home-suggestions";
  suggestions.setAttribute("role", "listbox");
  suggestions.hidden = true;
  input.parentElement.appendChild(suggestions);

  const renderSuggestions = () => {
    const value = input.value.trim().toLowerCase();
    if (!value) {
      suggestions.replaceChildren();
      suggestions.hidden = true;
      return;
    }
    const matches = data.packages.filter(item =>
      `${item.name} ${item.description}`.toLowerCase().includes(value)
    ).slice(0, 5);
    suggestions.innerHTML = matches.map(item =>
      `<a href="packages/package.html?name=${encodeURIComponent(item.name)}" role="option"><strong>${escape(item.name)}</strong><span>${escape(item.description)}</span></a>`
    ).join("");
    suggestions.hidden = matches.length === 0;
  };

  input.addEventListener("input", renderSuggestions);
  input.addEventListener("focus", renderSuggestions);
  document.addEventListener("click", event => {
    if (!form.contains(event.target)) suggestions.hidden = true;
  });
})();
