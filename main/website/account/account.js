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
const passwordPanel = $('#password-panel');
const passwordStatus = $('#password-status');
let schemaReady = false;
let captchaToken = '';
let recoveryMode = false;
const providerAvailability = new Map();
const secureContext = location.protocol === 'https:' || ['localhost', '127.0.0.1'].includes(location.hostname);
const pendingEmailKey = 'csp_pending_verification_email';

const reserved = new Set(['admin','administrator','moderator','staff','support','root','system','security','csp','cspfoundation','csp_foundation','official']);
const normalizedUsername = value => value.trim().toLowerCase();
signup.elements.birth_date.max = new Date().toISOString().slice(0, 10);
const message = (node, text, kind = '') => {
  node.textContent = text;
  node.className = `auth-status ${kind}`.trim();
};

const readableAuthError = error => {
  const raw = error?.message || 'The authentication service did not complete the request.';
  if (/email not confirmed/i.test(raw)) return 'Verify your email before signing in. Check your inbox and spam folder, or use Resend verification email.';
  if (/invalid login credentials/i.test(raw)) return 'Incorrect email or password. If this is a new account, verify the email first.';
  if (/user already registered/i.test(raw)) return 'An account already uses this email. Sign in or reset its password.';
  if (/rate limit/i.test(raw)) return 'Too many email requests. Wait a few minutes, then try again.';
  return raw;
};

async function withBusyButton(form, text, operation) {
  const button = form.querySelector('[type="submit"]');
  const original = button.textContent;
  button.disabled = true;
  button.textContent = text;
  try { return await operation(); }
  finally { button.disabled = false; button.textContent = original; }
}

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
  const blocking = [];
  if (!secureContext) blocking.push('HTTPS');
  if (!schemaReady) blocking.push('database migration');
  if (blocking.length || !config.turnstileSiteKey) {
    configNotice.hidden = false;
    configNotice.className = `auth-notice ${blocking.length ? 'error' : 'warning'}`;
    configNotice.textContent = blocking.length
      ? `Registration setup required: ${blocking.join(', ')}.`
      : 'Email verification is active. Automated CAPTCHA will activate when its public site key is configured.';
    signup.querySelector('.auth-submit').disabled = blocking.length > 0;
  }
  try {
    const response = await fetch(`${config.supabaseUrl}/auth/v1/settings`, { headers: { apikey: config.supabaseKey } });
    const settings = await response.json();
    document.querySelectorAll('[data-provider]').forEach(button => {
      const enabled = Boolean(settings.external?.[button.dataset.provider]);
      providerAvailability.set(button.dataset.provider, enabled);
      button.classList.toggle('unavailable', !enabled);
      button.setAttribute('aria-disabled', String(!enabled));
      button.title = enabled ? `Continue with ${button.textContent}` : `${button.textContent} is not enabled in Supabase yet`;
    });
  } catch {
    document.querySelectorAll('[data-provider]').forEach(button => providerAvailability.set(button.dataset.provider, false));
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

function requireCaptcha() {
  if (!config.turnstileSiteKey || captchaToken) return true;
  message(status, 'Complete the security check first.', 'error');
  document.querySelector('#turnstile-area')?.scrollIntoView({ behavior: 'smooth', block: 'center' });
  return false;
}

function resetCaptcha() {
  captchaToken = '';
  window.turnstile?.reset();
}

document.querySelectorAll('[data-auth-tab]').forEach(button => button.addEventListener('click', () => {
  document.querySelectorAll('[data-auth-tab]').forEach(item => item.classList.toggle('active', item === button));
  document.querySelectorAll('[data-auth-view]').forEach(view => { view.hidden = view.dataset.authView !== button.dataset.authTab; });
  message(status, '');
}));

signin.addEventListener('submit', async event => {
  event.preventDefault();
  if (!secureContext) return message(status, 'Sign-in requires HTTPS.', 'error');
  if (!requireCaptcha()) return;
  const values = Object.fromEntries(new FormData(signin));
  message(status, 'Signing in...');
  const { error } = await withBusyButton(signin, 'Signing in...', () => supabase.auth.signInWithPassword({
    email: values.email.trim(), password: values.password,
    options: { ...(captchaToken ? { captchaToken } : {}) }
  }));
  resetCaptcha();
  if (error) message(status, readableAuthError(error), 'error');
  else sessionStorage.removeItem(pendingEmailKey);
});

signup.addEventListener('submit', async event => {
  event.preventDefault();
  if (!secureContext || !schemaReady) return message(status, 'Secure registration is not configured yet.', 'error');
  const values = Object.fromEntries(new FormData(signup));
  const username = normalizedUsername(values.username);
  if (values.password !== values.confirm_password) return message(status, 'Passwords do not match.', 'error');
  if (!values.birth_date || values.birth_date > new Date().toISOString().slice(0, 10)) return message(status, 'Enter a valid birth date.', 'error');
  if (!/^[a-z0-9_]{3,30}$/.test(username) || reserved.has(username)) return message(status, 'Choose a valid, non-reserved username.', 'error');
  if (!requireCaptcha()) return;
  message(status, 'Checking username...');
  const { data: available, error: usernameError } = await supabase.rpc('username_available', { candidate: username });
  if (usernameError || !available) return message(status, usernameError ? 'Unable to verify that username.' : 'That username is unavailable.', 'error');
  const { error } = await withBusyButton(signup, 'Creating account...', () => supabase.auth.signUp({
    email: values.email.trim(), password: values.password,
    options: { ...(captchaToken ? { captchaToken } : {}), emailRedirectTo: `${location.origin}/account/`, data: {
      username, first_name: values.first_name.trim(), last_name: values.last_name.trim(),
      birth_date: values.birth_date, bio: values.bio.trim()
    }}
  }));
  resetCaptcha();
  if (error) return message(status, readableAuthError(error), 'error');
  sessionStorage.setItem(pendingEmailKey, values.email.trim());
  signup.reset();
  message(status, 'Account created. Check your email to verify it before signing in.', 'success');
});

$('#forgot-password').addEventListener('click', async () => {
  if (!secureContext) return message(status, 'Password recovery requires HTTPS.', 'error');
  const email = signin.elements.email.value.trim();
  if (!email) return message(status, 'Enter your email address first.', 'error');
  if (!requireCaptcha()) return;
  const { error } = await supabase.auth.resetPasswordForEmail(email, {
    redirectTo: `${location.origin}/account/`, ...(captchaToken ? { captchaToken } : {})
  });
  resetCaptcha();
  message(status, error ? readableAuthError(error) : 'If the account exists, a recovery email has been sent.', error ? 'error' : 'success');
});

$('#resend-verification').addEventListener('click', async () => {
  if (!secureContext) return message(status, 'Email verification requires HTTPS.', 'error');
  const email = signin.elements.email.value.trim() || sessionStorage.getItem(pendingEmailKey) || '';
  if (!email) return message(status, 'Enter your email address first.', 'error');
  if (!requireCaptcha()) return;
  message(status, 'Sending verification email...');
  const { error } = await supabase.auth.resend({
    type: 'signup', email,
    options: { emailRedirectTo: `${location.origin}/account/`, ...(captchaToken ? { captchaToken } : {}) }
  });
  resetCaptcha();
  message(status, error ? readableAuthError(error) : 'Verification email sent. Check your inbox and spam folder.', error ? 'error' : 'success');
});

document.querySelectorAll('[data-provider]').forEach(button => button.addEventListener('click', async () => {
  if (!secureContext) return message(status, 'Social sign-in requires HTTPS.', 'error');
  if (!providerAvailability.get(button.dataset.provider)) return message(status, `${button.textContent} sign-in must be enabled by the site administrator in Supabase first.`, 'error');
  const { error } = await supabase.auth.signInWithOAuth({ provider: button.dataset.provider, options: { redirectTo: `${location.origin}/account/` } });
  if (error) message(status, readableAuthError(error), 'error');
}));

async function renderSession(session) {
  if (recoveryMode) return;
  passwordPanel.hidden = true;
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
  const { data: profile } = await supabase.from('profiles').select('username,display_name,bio,avatar_path,moderation_status').eq('id', user.id).maybeSingle();
  if (profile) {
    $('#profile-heading').textContent = profile.display_name || profile.username;
    $('#profile-form').elements.username.value = profile.username || '';
    $('#profile-form').elements.display_name.value = profile.display_name || '';
    $('#profile-form').elements.bio.value = profile.bio || '';
    const suspended = profile.moderation_status === 'suspended';
    [...$('#profile-form').elements].forEach(control => { control.disabled = suspended; });
    if (suspended) message(profileStatus, 'This account is suspended. The public decision is available in banned.json.', 'error');
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
$('#password-form').addEventListener('submit', async event => {
  event.preventDefault();
  const values = Object.fromEntries(new FormData(event.currentTarget));
  if (values.password !== values.confirm_password) return message(passwordStatus, 'Passwords do not match.', 'error');
  const { error } = await supabase.auth.updateUser({ password: values.password });
  if (error) return message(passwordStatus, error.message, 'error');
  event.currentTarget.reset();
  recoveryMode = false;
  message(passwordStatus, 'Password updated. You can continue using your account.', 'success');
  renderSession((await supabase.auth.getSession()).data.session);
});
supabase.auth.onAuthStateChange((event, session) => setTimeout(() => {
  if (event === 'PASSWORD_RECOVERY') {
    recoveryMode = true;
    authPanel.hidden = true;
    profilePanel.hidden = true;
    passwordPanel.hidden = false;
  } else renderSession(session);
}, 0));
inspectBackend().then(() => { const timer = setInterval(() => { if (window.turnstile) { clearInterval(timer); renderTurnstile(); } }, 100); setTimeout(() => clearInterval(timer), 10000); });
supabase.auth.getSession().then(({ data }) => renderSession(data.session));
