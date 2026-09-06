"use strict";

(() => {
  const posts = document.querySelector("#wiki-posts");
  const form = document.querySelector("#post-form");
  const status = document.querySelector("#post-status");
  const search = document.querySelector("#wiki-search");
  if (!posts || !form) return;

  const escape = value => String(value).replace(/[&<>"']/g, character => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  }[character]));
  let allPosts = [];

  const render = () => {
    const needle = (search?.value || "").trim().toLowerCase();
    const visible = allPosts.filter(post => `${post.title} ${post.category} ${post.body}`.toLowerCase().includes(needle));
    posts.innerHTML = visible.length ? visible.map(post => `<article class="wiki-post"><h3>${escape(post.title)}</h3><small>${escape(post.category)} · ${escape(post.author)} · ${escape(post.created)}</small><p>${escape(post.body)}</p></article>`).join("") : "<p>No wiki posts match this search.</p>";
  };

  const load = () => fetch("/api/wiki/posts").then(response => response.json()).then(data => { allPosts = data.posts || []; render(); });
  search?.addEventListener("input", render);
  form.addEventListener("submit", event => {
    event.preventDefault();
    status.textContent = "Publishing...";
    fetch("/api/wiki/posts", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(Object.fromEntries(new FormData(form))) })
      .then(response => response.json().then(data => ({ ok: response.ok, data })))
      .then(({ ok, data }) => { if (!ok) throw new Error(data.error || "Unable to publish"); form.reset(); status.textContent = "Post published."; allPosts.unshift(data.post); render(); })
      .catch(error => { status.textContent = error.message; });
  });
  load().catch(() => { posts.innerHTML = "<p>Wiki posts are temporarily unavailable.</p>"; });
})();
