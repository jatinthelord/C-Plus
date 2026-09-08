"use strict";

(async () => {
  const data = window.CSP_RELEASE_DATA || { current: "", artifacts: [], packages: [], libraries: [], releases: [] };
  const body = document.body;
  const base = body.dataset.base || "";
  const page = body.dataset.page || "home";
  try {
    const response = await fetch(`${base}data/csx-index.json`, { cache: 'no-cache' });
    if (response.ok) {
      const registry = await response.json();
      const previous = new Map(data.packages.map(item => [item.name, item]));
      data.packages = registry.packages.map(item => {
        const saved = previous.get(item.name) || {};
        return {
          arch: 'any', repository: 'Extra', license: 'MIT', installedSize: 'Source package',
          maintainer: 'CSP Foundation', updated: '2026-09-08', artifacts: [],
          ...saved,
          name: item.name,
          version: saved.version || `${item.version}-1`,
          description: item.description,
          dependencies: item.depends || [],
          provides: (item.headers || []).join(', ') || saved.provides || item.name,
          files: (item.headers || []).map(header => `usr/include/${header}`)
        };
      });
      const headers = new Map();
      registry.packages.forEach(item => (item.headers || []).forEach(header => {
        if (!headers.has(header)) headers.set(header, { name: `<${header}>`, package: item.name, description: item.description });
      }));
      data.libraries = [...headers.values()];
    }
  } catch { /* Keep the embedded release data as an offline fallback. */ }
  const $ = selector => document.querySelector(selector);
  const $$ = selector => [...document.querySelectorAll(selector)];
  const escape = value => String(value).replace(/[&<>"']/g, character => ({ "&":"&amp;", "<":"&lt;", ">":"&gt;", '"':"&quot;", "'":"&#39;" }[character]));
  const bytes = value => value >= 1048576 ? `${(value / 1048576).toFixed(1)} MiB` : value >= 1024 ? `${(value / 1024).toFixed(1)} KiB` : `${value} B`;

  $$('[data-current-version]').forEach(node => { if (data.current) node.textContent = data.current; });

  const menu = $('.menu-button');
  const links = $('#nav-links');
  if (menu && links) menu.addEventListener('click', () => {
    const open = links.classList.toggle('open');
    menu.setAttribute('aria-expanded', String(open));
  });

  if (!document.cookie.split('; ').some(value => value.startsWith('csp_cookie_choice='))) {
    const banner = document.createElement('dialog');
    banner.className = 'cookie-banner';
    banner.open = true;
    banner.setAttribute('aria-label', 'Cookie notice');
    banner.innerHTML = '<strong>Essential storage only</strong><p>CSP uses one preference cookie and Supabase browser storage for sign-in. There are no advertising cookies.</p><button class="button primary" type="button">Accept essential</button> <a href="' + base + 'privacy/">Privacy details</a>';
    banner.querySelector('button').addEventListener('click', () => {
      const secure = location.protocol === 'https:' ? '; Secure' : '';
      document.cookie = `csp_cookie_choice=essential; Path=/; Max-Age=31536000; SameSite=Lax${secure}`;
      banner.remove();
    });
    document.body.append(banner);
  }

  document.addEventListener('click', async event => {
    const button = event.target.closest('[data-copy]');
    if (!button) return;
    const original = button.textContent;
    try { await navigator.clipboard.writeText(button.dataset.copy); button.textContent = 'Copied'; }
    catch { button.textContent = 'Select'; }
    setTimeout(() => { button.textContent = original; }, 1200);
  });

  const downloadList = $('#download-list');
  const renderDownloads = (platform = 'all') => {
    if (!downloadList) return;
    const artifacts = data.artifacts.filter(artifact => platform === 'all' || artifact.platform === platform);
    downloadList.innerHTML = artifacts.length ? artifacts.map(artifact => `
      <article class="download-item">
        <div><div class="artifact-name">${escape(artifact.name)}</div><div class="artifact-meta">${escape(artifact.platformLabel)} · ${bytes(artifact.bytes)}</div></div>
        <div class="artifact-meta">v${escape(artifact.version)}</div>
        <div class="hash" title="SHA-256: ${escape(artifact.sha256)}"><code>${escape(artifact.sha256)}</code><button data-copy="${escape(artifact.sha256)}">Copy</button></div>
        <a class="download-link" href="${base}${escape(artifact.url)}" download>Download</a>
      </article>`).join('') : '<p class="empty">No artifacts match this platform.</p>';
  };
  renderDownloads();
  $$('.tab').forEach(tab => tab.addEventListener('click', () => {
    $$('.tab').forEach(item => item.classList.remove('active'));
    tab.classList.add('active');
    renderDownloads(tab.dataset.platform);
  }));

  const packageBody = $('#package-table');
  const packageUrl = name => page === 'packages' ? `package.html?name=${encodeURIComponent(name)}` : `${base}packages/package.html?name=${encodeURIComponent(name)}`;
  const renderPackages = (query = '') => {
    if (!packageBody) return;
    const normalized = query.trim().toLowerCase();
    const repo = $('#repo-filter')?.value || 'all';
    const arch = $('#arch-filter')?.value || 'all';
    const found = data.packages.filter(item => {
      const text = `${item.name} ${item.description} ${item.provides || ''}`.toLowerCase();
      return text.includes(normalized) && (repo === 'all' || item.repository === repo) && (arch === 'all' || item.arch === arch);
    });
    packageBody.innerHTML = found.map(item => `<tr><td>${escape(item.arch)}</td><td>${escape(item.repository)}</td><td class="pkg-name"><a href="${packageUrl(item.name)}">${escape(item.name)}</a></td><td>${escape(item.version)}</td><td class="wrap">${escape(item.description)}</td><td>${escape(item.updated)}</td></tr>`).join('');
    const count = $('#package-count');
    if (count) count.textContent = `${found.length} matching package${found.length === 1 ? '' : 's'} found.`;
  };
  const initialQuery = new URLSearchParams(location.search).get('q') || '';
  if ($('#package-search')) $('#package-search').value = initialQuery;
  renderPackages(initialQuery);
  $('#package-search')?.addEventListener('input', event => renderPackages(event.target.value));
  $('#repo-filter')?.addEventListener('change', () => renderPackages($('#package-search')?.value || ''));
  $('#arch-filter')?.addEventListener('change', () => renderPackages($('#package-search')?.value || ''));

  const libraryGrid = $('#library-grid');
  if (libraryGrid) libraryGrid.innerHTML = data.libraries.map(item => `<a class="library-card" href="${base}packages/package.html?name=${encodeURIComponent(item.package)}"><code>${escape(item.name)}</code><p>${escape(item.description)}</p><span>Package details →</span></a>`).join('');

  const timeline = $('#release-timeline');
  if (timeline) timeline.innerHTML = data.releases.map(item => `<article class="release-entry"><h3>CSP ${escape(item.version)} <span class="badge">${escape(item.channel)}</span></h3><time datetime="${escape(item.date)}">${escape(item.date)}</time><p>${escape(item.summary)}</p></article>`).join('');

  const detail = $('#package-detail');
  if (!detail) return;
  const name = new URLSearchParams(location.search).get('name') || 'csp';
  const item = data.packages.find(candidate => candidate.name === name);
  if (!item) {
    document.title = 'Package not found — CSP';
    detail.innerHTML = '<div class="box"><h1>Package not found</h1><p>The requested CSP package does not exist.</p></div>';
    return;
  }
  document.title = `${item.name} ${item.version} — CSP Packages`;
  const related = data.artifacts.filter(artifact => (item.artifacts || []).includes(artifact.name));
  detail.innerHTML = `
    <div class="breadcrumb"><a href="index.html">Packages</a> / ${escape(item.repository)} / ${escape(item.arch)} / ${escape(item.name)}</div>
    <article class="box package-detail">
      <header><div><p class="eyebrow">${escape(item.repository)} · ${escape(item.arch)}</p><h1>${escape(item.name)} <span>${escape(item.version)}</span></h1><p>${escape(item.description)}</p></div><span class="package-state">Up to date</span></header>
      <div class="detail-columns"><dl>
        <div><dt>Architecture</dt><dd>${escape(item.arch)}</dd></div><div><dt>Repository</dt><dd>${escape(item.repository)}</dd></div><div><dt>License</dt><dd>${escape(item.license)}</dd></div><div><dt>Installed size</dt><dd>${escape(item.installedSize)}</dd></div><div><dt>Packager</dt><dd>${escape(item.maintainer)}</dd></div><div><dt>Last updated</dt><dd>${escape(item.updated)}</dd></div>
      </dl><section><h2>Dependencies</h2><ul>${(item.dependencies || []).map(value => `<li>${escape(value)}</li>`).join('') || '<li>None</li>'}</ul><h2>Provides</h2><p>${escape(item.provides || '—')}</p></section></div>
      <section class="detail-section"><h2>Package files</h2><pre>${(item.files || []).map(escape).join('\n')}</pre></section>
      <section class="detail-section"><h2>Downloads and checksums</h2>${related.map(artifact => `<div class="detail-artifact"><a href="${base}${escape(artifact.url)}" download>${escape(artifact.name)}</a><code>${escape(artifact.sha256)}</code><button data-copy="${escape(artifact.sha256)}">Copy hash</button></div>`).join('') || '<p>No direct artifact for this split package.</p>'}</section>
    </article>`;
})();
