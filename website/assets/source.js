"use strict";

(() => {
  const list = document.querySelector("#source-list");
  const viewer = document.querySelector("#source-viewer");
  const title = document.querySelector("#source-title");
  if (!list || !viewer) return;

  const escape = value => String(value).replace(/[&<>"']/g, character => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  }[character]));

  const loadFile = async file => {
    const response = await fetch(`/api/source?file=${encodeURIComponent(file)}`);
    const result = await response.json();
    if (!response.ok) throw new Error(result.error || "Unable to load source");
    title.textContent = result.file;
    viewer.textContent = result.content;
    document.querySelectorAll("#source-list a").forEach(link => {
      link.classList.toggle("active", link.dataset.file === file);
    });
  };

  const render = files => {
    list.innerHTML = files.map(file =>
      `<li><a href="?file=${encodeURIComponent(file)}" data-file="${escape(file)}">${escape(file)}</a></li>`
    ).join("");
    list.addEventListener("click", event => {
      const link = event.target.closest("a[data-file]");
      if (!link) return;
      event.preventDefault();
      history.replaceState({}, "", link.href);
      loadFile(link.dataset.file).catch(error => { viewer.textContent = error.message; });
    });
  };

  fetch("/api/libraries")
    .then(response => response.json())
    .then(result => render(result.headers))
    .then(() => {
      const requested = new URLSearchParams(location.search).get("file");
      const first = requested || list.querySelector("a")?.dataset.file;
      if (first) return loadFile(first);
    })
    .catch(error => { list.innerHTML = `<li>${escape(error.message)}</li>`; });
})();
