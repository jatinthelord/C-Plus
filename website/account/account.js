import { createClient } from "https://cdn.jsdelivr.net/npm/@supabase/supabase-js@2/+esm";

const config = window.CSP_AUTH_CONFIG || {};
const supabase = createClient(config.supabaseUrl, config.supabaseKey, {
  auth: { persistSession: true, autoRefreshToken: true, detectSessionInUrl: true }
});
const $ = selector => document.querySelector(selector);
const status = $('#auth-status');
const profileStatus = $('#profile-status');
const signup = $('#signup-form');
const signin = $('#signin-form');
const authPanel = $('#auth-panel');
const profilePanel = $('#profile-panel');
const configNotice = $('#auth-config-status');
let schemaReady = false;
let captchaToken = '';
const secureContext = location.protocol === 'https:' || ['localhost', '127.0.0.1'].includes(location.hostname);

const reserved = new Set(['admin','administrator','moderator','staff','support','root','system','security','csp','cspfoundation','csp_foundation','official']);
const normalizedUsername = value => value.trim().toLowerCase();
const message = (node, text, kind = '') => {
  node.textContent = text;
  node.className = `auth-status ${kind}`.trim();
};

function identicon(target, seed) {
  target.className = 'avatar';
  target.replaceChildren();
  let hash = 2166136261;
  for (const character of seed) hash = Math.imul(hash ^ character.charCodeAt(0), 16777619);
  const hue = Math.abs(hash) % 360;
  for (let row = 0; row < 5; row++) for (let column = 0; column < 5; column++) {
    const mirror = column > 2 ? 4 - column : column;
    const bit = (hash >>> ((row * 3 + mirror) % 24)) & 1;
    const cell = document.createElement('i');
    if (bit) cell.style.background = `hsl(${hue} 58% 48%)`;
    target.append(cell);
  }
}

async function inspectBackend() {
  const { error } = await supabase.from('profiles').select('id', { head: true, count: 'exact' }).limit(1);
  schemaReady = !error;
  const missing = [];
  if (!secureContext) missing.push('HTTPS');
  if (!schemaReady) missing.push('database migration');
  if (!config.turnstileSiteKey) missing.push('Turnstile site key');
  if (missing.length) {
    configNotice.hidden = false;
    configNotice.className = 'auth-notice error';
    configNotice.textContent = `Registration setup required: ${missing.join(', ')}.`;
    signup.querySelector('.auth-submit').disabled = true;
  }
  try {
    const response = await fetch(`${config.supabaseUrl}/auth/v1/settings`, { headers: { apikey: config.supabaseKey } });
    const settings = await response.json();
    document.querySelectorAll('[data-provider]').forEach(button => {
      const enabled = Boolean(settings.external?.[button.dataset.provider]);
      button.disabled = !enabled;
      button.title = enabled ? `Continue with ${button.textContent}` : `${button.textContent} is not enabled in Supabase yet`;
    });
  } catch {
    document.querySelectorAll('[data-provider]').forEach(button => { button.disabled = true; });
  }
}

function renderTurnstile() {
  if (!config.turnstileSiteKey || !window.turnstile) return;
  window.turnstile.render('#turnstile-widget', {
    sitekey: config.turnstileSiteKey,
    theme: 'light',
    action: 'signup',
    callback: token => { captchaToken = token; },
    'expired-callback': () => { captchaToken = ''; }
  });
}

document.querySelectorAll('[data-auth-tab]').forEach(button => button.addEventListener('click', () => {
  document.querySelectorAll('[data-auth-tab]').forEach(item => item.classList.toggle('active', item === button));
  document.querySelectorAll('[data-auth-view]').forEach(view => { view.hidden = view.dataset.authView !== button.dataset.authTab; });
  message(status, '');
}));

signin.addEventListener('submit', async event => {
  event.preventDefault();
  if (!secureContext) return message(status, 'Sign-in requires HTTPS.', 'error');
  const values = Object.fromEntries(new FormData(signin));
  message(status, 'Signing in…');
  const { error } = await supabase.auth.signInWithPassword({ email: values.email.trim(), password: values.password });
  if (error) message(status, error.message, 'error');
});

signup.addEventListener('submit', async event => {
  event.preventDefault();
  if (!schemaReady || !config.turnstileSiteKey) return message(status, 'Secure registration is not configured yet.', 'error');
  const values = Object.fromEntries(new FormData(signup));
  const username = normalizedUsername(values.username);
  if (values.password !== values.confirm_password) return message(status, 'Passwords do not match.', 'error');
  if (!/^[a-z0-9_]{3,30}$/.test(username) || reserved.has(username)) return message(status, 'Choose a valid, non-reserved username.', 'error');
  if (!captchaToken) return message(status, 'Complete the security challenge.', 'error');
  message(status, 'Checking username…');
  const { data: available, error: usernameError } = await supabase.rpc('username_available', { candidate: username });
  if (usernameError || !available) return message(status, usernameError ? 'Unable to verify that username.' : 'That username is unavailable.', 'error');
  const { error } = await supabase.auth.signUp({
    email: values.email.trim(), password: values.password,
    options: { captchaToken, emailRedirectTo: `${location.origin}/account/`, data: {
      username, first_name: values.first_name.trim(), last_name: values.last_name.trim(),
      birth_date: values.birth_date, bio: values.bio.trim()
    }}
  });
  captchaToken = '';
  window.turnstile?.reset();
  if (error) return message(status, error.message, 'error');
  signup.reset();
  message(status, 'Account created. Check your email to verify it before signing in.', 'success');
});

$('#forgot-password').addEventListener('click', async () => {
  const email = signin.elements.email.value.trim();
  if (!email) return message(status, 'Enter your email address first.', 'error');
  const { error } = await supabase.auth.resetPasswordForEmail(email, { redirectTo: `${location.origin}/account/` });
  message(status, error ? error.message : 'If the account exists, a recovery email has been sent.', error ? 'error' : 'success');
});

document.querySelectorAll('[data-provider]').forEach(button => button.addEventListener('click', async () => {
  const { error } = await supabase.auth.signInWithOAuth({ provider: button.dataset.provider, options: { redirectTo: `${location.origin}/account/` } });
  if (error) message(status, error.message, 'error');
}));

async function renderSession(session) {
  if (!session?.user) {
    authPanel.hidden = false;
    profilePanel.hidden = true;
    return;
  }
  authPanel.hidden = true;
  profilePanel.hidden = false;
  const user = session.user;
  $('#profile-email').textContent = user.email || '';
  identicon($('#profile-avatar'), user.id);
  const { data: profile } = await supabase.from('profiles').select('username,display_name,bio,avatar_path').eq('id', user.id).maybeSingle();
  if (profile) {
    $('#profile-heading').textContent = profile.display_name || profile.username;
    $('#profile-form').elements.username.value = profile.username || '';
    $('#profile-form').elements.display_name.value = profile.display_name || '';
    $('#profile-form').elements.bio.value = profile.bio || '';
    if (profile.avatar_path) {
      const { data } = supabase.storage.from('avatars').getPublicUrl(profile.avatar_path);
      const avatar = $('#profile-avatar');
      avatar.replaceChildren(); avatar.className = 'avatar has-image'; avatar.style.backgroundImage = `url("${data.publicUrl}")`;
    }
  }
}

$('#profile-form').addEventListener('submit', async event => {
  event.preventDefault();
  const { data: { user } } = await supabase.auth.getUser();
  if (!user) return;
  const form = event.currentTarget;
  const username = normalizedUsername(form.elements.username.value);
  if (!/^[a-z0-9_]{3,30}$/.test(username) || reserved.has(username)) return message(profileStatus, 'Choose a valid, non-reserved username.', 'error');
  let avatarPath;
  const file = form.elements.avatar.files[0];
  if (file) {
    if (file.size > 2 * 1024 * 1024 || !['image/jpeg','image/png','image/webp'].includes(file.type)) return message(profileStatus, 'Avatar must be JPEG, PNG, or WebP and no larger than 2 MiB.', 'error');
    const extension = file.type === 'image/jpeg' ? 'jpg' : file.type.split('/')[1];
    avatarPath = `${user.id}/avatar.${extension}`;
    const { error } = await supabase.storage.from('avatars').upload(avatarPath, file, { upsert: true, contentType: file.type });
    if (error) return message(profileStatus, error.message, 'error');
  }
  const update = { username, display_name: form.elements.display_name.value.trim(), bio: form.elements.bio.value.trim(), updated_at: new Date().toISOString() };
  if (avatarPath) update.avatar_path = avatarPath;
  const { error } = await supabase.from('profiles').update(update).eq('id', user.id);
  message(profileStatus, error ? error.message : 'Profile saved.', error ? 'error' : 'success');
  if (!error) renderSession((await supabase.auth.getSession()).data.session);
});

$('#sign-out').addEventListener('click', () => supabase.auth.signOut());
supabase.auth.onAuthStateChange((_event, session) => setTimeout(() => renderSession(session), 0));
inspectBackend().then(() => { const timer = setInterval(() => { if (window.turnstile) { clearInterval(timer); renderTurnstile(); } }, 100); setTimeout(() => clearInterval(timer), 10000); });
supabase.auth.getSession().then(({ data }) => renderSession(data.session));
