import { createClient } from "https://cdn.jsdelivr.net/npm/@supabase/supabase-js@2/+esm";

const posts = document.querySelector('#wiki-posts');
const search = document.querySelector('#wiki-search');
const postForm = document.querySelector('#wiki-post-form');
const accountLink = document.querySelector('#wiki-account-link');
const postStatus = document.querySelector('#wiki-post-status');
const config = window.CSP_AUTH_CONFIG || {};
const supabase = createClient(config.supabaseUrl, config.supabaseKey, {
  auth: { persistSession: true, autoRefreshToken: true, detectSessionInUrl: true }
});
const escape = value => String(value).replace(/[&<>"']/g, character => ({
  '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'
}[character]));
let allPosts = [];

function render() {
  const needle = search.value.trim().toLowerCase();
  document.querySelectorAll('.docs-card, .docs-faq details').forEach(item => {
    item.hidden = Boolean(needle) && !item.textContent.toLowerCase().includes(needle);
  });
  const visible = allPosts.filter(post => `${post.title} ${post.category} ${post.body}`.toLowerCase().includes(needle));
  posts.innerHTML = visible.length ? visible.map(post => {
    const state = post.status === 'pending' ? '<span class="post-state">Awaiting review</span>' : '';
    return `<article class="docs-post"><h3>${escape(post.title)} ${state}</h3><small>${escape(post.category)} · ${escape(post.author)} · ${escape(post.created)}</small><p>${escape(post.body)}</p></article>`;
  }).join('') : `<p>${needle ? 'No wiki posts match this search.' : 'No community posts published yet.'}</p>`;
}

async function loadPosts() {
  let staticPosts = [];
  try {
    const response = await fetch('posts.json');
    const data = await response.json();
    staticPosts = (data.posts || data || []).map(post => ({ ...post, status: 'published' }));
  } catch { /* Database posts can still load. */ }

  const { data, error } = await supabase
    .from('wiki_posts')
    .select('id,title,category,body,status,created_at,profiles!wiki_posts_author_id_fkey(username,display_name)')
    .order('created_at', { ascending: false })
    .limit(100);
  const databasePosts = error ? [] : data.map(post => ({
    ...post,
    author: post.profiles?.display_name || post.profiles?.username || 'CSP member',
    created: new Date(post.created_at).toLocaleDateString()
  }));
  allPosts = [...databasePosts, ...staticPosts];
  render();
}

function showSession(session) {
  const signedIn = Boolean(session?.user);
  postForm.hidden = !signedIn;
  accountLink.hidden = signedIn;
}

postForm.addEventListener('submit', async event => {
  event.preventDefault();
  const { data: { user } } = await supabase.auth.getUser();
  if (!user) {
    postStatus.textContent = 'Sign in before posting.';
    return;
  }
  const values = Object.fromEntries(new FormData(postForm));
  postStatus.textContent = 'Running server moderation checks...';
  const { data, error } = await supabase.from('wiki_posts').insert({
    author_id: user.id,
    title: values.title.trim(),
    category: values.category,
    body: values.body.trim()
  }).select('status').single();
  if (error) {
    postStatus.textContent = error.message.includes('schema cache')
      ? 'Wiki posting needs the latest database migration.'
      : error.message;
    return;
  }
  postForm.reset();
  postStatus.textContent = data.status === 'published'
    ? 'Post published.'
    : 'The safety checks flagged this post for human review. Your account was not banned.';
  await loadPosts();
});

search.addEventListener('input', render);
document.addEventListener('keydown', event => {
  if (event.key === '/' && document.activeElement !== search && !event.target.closest('input, textarea, select')) {
    event.preventDefault();
    search.focus();
  }
});
supabase.auth.onAuthStateChange((_event, session) => showSession(session));
supabase.auth.getSession().then(({ data }) => showSession(data.session));
loadPosts();
