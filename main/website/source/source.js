"use strict";

(() => {
  const list = document.querySelector('#source-list');
  const viewer = document.querySelector('#source-viewer code');
  const pathLabel = document.querySelector('#source-path');
  const meta = document.querySelector('#source-meta');
  const raw = document.querySelector('#source-raw');
  const search = document.querySelector('#source-search');
  const roots = document.querySelector('#source-roots');
  const count = document.querySelector('#source-count');
  const generated = document.querySelector('#source-generated');
  let files = [];
  let selectedRoot = 'all';

  const escape = value => String(value).replace(/[&<>"']/g, character => ({
    '&':'&amp;', '<':'&lt;', '>':'&gt;', '"':'&quot;', "'":'&#39;'
  }[character]));
  const fileUrl = file => `../code/${file.split('/').map(encodeURIComponent).join('/')}`;

  function visibleFiles() {
    const needle = search.value.trim().toLowerCase();
    return files.filter(file => {
      const root = file.includes('/') ? file.split('/')[0] : 'root';
      return (selectedRoot === 'all' || root === selectedRoot) && file.toLowerCase().includes(needle);
    });
  }

  function renderList() {
    const matches = visibleFiles();
    const shown = matches.slice(0, 500);
    list.innerHTML = shown.map(file => `<li><a href="?file=${encodeURIComponent(file)}" data-file="${escape(file)}">${escape(file)}</a></li>`).join('') || '<li>No source file matches.</li>';
    meta.textContent = matches.length > shown.length ? `Showing 500 of ${matches.length} matches` : `${matches.length} matching files`;
  }

  async function loadFile(file) {
    if (!files.includes(file)) throw new Error('That file is not in the deployed source manifest.');
    viewer.textContent = 'Loading…';
    const response = await fetch(fileUrl(file));
    if (!response.ok) throw new Error(`Unable to load ${file}`);
    const content = await response.text();
    pathLabel.textContent = file;
    meta.textContent = `${new Blob([content]).size.toLocaleString()} bytes`;
    viewer.textContent = content;
    raw.href = fileUrl(file);
    raw.download = file.split('/').pop();
    raw.hidden = false;
    document.querySelectorAll('#source-list a').forEach(link => link.classList.toggle('active', link.dataset.file === file));
  }

  list.addEventListener('click', event => {
    const link = event.target.closest('a[data-file]');
    if (!link) return;
    event.preventDefault();
    history.replaceState({}, '', link.href);
    loadFile(link.dataset.file).catch(error => { viewer.textContent = error.message; });
  });
  search.addEventListener('input', renderList);

  fetch('../data/source-manifest.json')
    .then(response => {
      if (!response.ok) throw new Error('Source manifest is unavailable.');
      return response.json();
    })
    .then(manifest => {
      files = manifest.files || [];
      count.textContent = files.length.toLocaleString();
      generated.textContent = manifest.generated ? `Synced ${new Date(manifest.generated).toLocaleString()}` : '';
      const folders = [...new Set(files.map(file => file.includes('/') ? file.split('/')[0] : 'root'))].sort();
      roots.innerHTML = ['all', ...folders].map(folder => `<button type="button" data-root="${escape(folder)}" class="${folder === 'all' ? 'active' : ''}">${escape(folder)}</button>`).join('');
      roots.addEventListener('click', event => {
        const button = event.target.closest('[data-root]');
        if (!button) return;
        selectedRoot = button.dataset.root;
        roots.querySelectorAll('button').forEach(item => item.classList.toggle('active', item === button));
        renderList();
      });
      renderList();
      const requested = new URLSearchParams(location.search).get('file');
      const first = requested && files.includes(requested) ? requested : files.find(file => file === 'README.md') || files[0];
      if (first) loadFile(first).catch(error => { viewer.textContent = error.message; });
    })
    .catch(error => { list.innerHTML = `<li>${escape(error.message)}</li>`; viewer.textContent = error.message; });
})();
