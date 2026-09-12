"use strict";

(async () => {
  const base = document.body.dataset.base || "";
  const status = document.querySelector("#catalog-status");
  const table = document.querySelector("#catalog-table");
  const search = document.querySelector("#catalog-search");
  const kind = document.querySelector("#catalog-kind");
  let catalog = { packages: [], libraries: [], sources: [] };

  const escape = value => String(value).replace(/[&<>"']/g, ch => (
    { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[ch]
  ));

  async function loadJson(path, fallback) {
    const response = await fetch(`${base}${path}`, { cache: "no-cache" });
    if (!response.ok) return fallback;
    return response.json();
  }

  const manifest = await loadJson("data/manifest.json", null);
  if (!manifest) {
    if (status) status.textContent = "Catalog data has not been generated. Run python tools/build_site_catalog.py";
    return;
  }
  const [packages, libraries, sources] = await Promise.all([
    loadJson("data/packages.json", { packages: [] }),
    loadJson("data/libraries.json", { libraries: [] }),
    loadJson("data/sources.json", { sources: [] })
  ]);
  catalog = {
    packages: packages.packages || [],
    libraries: libraries.libraries || [],
    sources: sources.sources || []
  };
  if (status) {
    status.textContent =
      `${manifest.packages} packages · ${manifest.libraries} libraries · ${manifest.sources} source files`;
  }
  const pkgCount = document.querySelector("[data-catalog-packages]");
  const libCount = document.querySelector("[data-catalog-libraries]");
  const srcCount = document.querySelector("[data-catalog-sources]");
  if (pkgCount) pkgCount.textContent = String(manifest.packages);
  if (libCount) libCount.textContent = String(manifest.libraries);
  if (srcCount) srcCount.textContent = String(manifest.sources);

  function rows() {
    const needle = (search?.value || "").trim().toLowerCase();
    const selected = kind?.value || "packages";
    if (selected === "libraries") {
      return catalog.libraries
        .filter(item => `${item.name} ${item.path} ${item.summary}`.toLowerCase().includes(needle))
        .map(item => [item.name, item.path, `${item.bytes} B`, item.summary]);
    }
    if (selected === "sources") {
      return catalog.sources
        .filter(item => item.file.toLowerCase().includes(needle))
        .map(item => [item.file, item.sha256.slice(0, 12), `${item.bytes} B`, "tracked source"]);
    }
    return catalog.packages
      .filter(item => `${item.name} ${item.description}`.toLowerCase().includes(needle))
      .map(item => [item.name, item.version, item.repository, item.description]);
  }

  function render() {
    if (!table) return;
    const found = rows();
    table.innerHTML = found.map(cols =>
      `<tr>${cols.map(col => `<td>${escape(col)}</td>`).join("")}</tr>`
    ).join("") || `<tr><td colspan="4">No catalog entries match.</td></tr>`;
  }

  search?.addEventListener("input", render);
  kind?.addEventListener("change", render);
  render();
})();
