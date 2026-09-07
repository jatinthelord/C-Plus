"use strict";

(() => {
  const posts = document.querySelector("#wiki-posts");
  const search = document.querySelector("#wiki-search");
  if (!posts || !search) return;
  const escape = value => String(value).replace(/[&<>"']/g, character => ({ "&":"&amp;", "<":"&lt;", ">":"&gt;", '"':"&quot;", "'":"&#39;" }[character]));
  let allPosts = [];

  const render = () => {
    const needle = search.value.trim().toLowerCase();
    document.querySelectorAll(".docs-card, .docs-faq details").forEach(item => {
      item.hidden = Boolean(needle) && !item.textContent.toLowerCase().includes(needle);
    });
    const visible = allPosts.filter(post => `${post.title} ${post.category} ${post.body}`.toLowerCase().includes(needle));
    posts.innerHTML = visible.length ? visible.map(post => `<article class="docs-post"><h3>${escape(post.title)}</h3><small>${escape(post.category)} · ${escape(post.author)} · ${escape(post.created)}</small><p>${escape(post.body)}</p></article>`).join("") : `<p>${needle ? "No project updates match this search." : "No project updates published yet."}</p>`;
  };

  search.addEventListener("input", render);
  document.addEventListener("keydown", event => {
    if (event.key === "/" && document.activeElement !== search) { event.preventDefault(); search.focus(); }
  });
  fetch("posts.json").then(response => response.json()).then(data => { allPosts = data.posts || data || []; render(); }).catch(() => { posts.innerHTML = "<p>Project updates are temporarily unavailable.</p>"; });
})();
