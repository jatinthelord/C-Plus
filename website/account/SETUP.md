# CSP Foundation account setup

The browser uses the public Supabase publishable key in `assets/auth-config.js`. Never put a service-role key, database password, OAuth client secret, or Turnstile secret in this repository.

1. Open the Supabase SQL editor for project `zaanoqtzttbbtjgbwhch` and run `supabase/migrations/20260907_auth.sql` from the repository root.
2. Under Authentication > URL Configuration, set the site URL to `https://cspfoundations.org` and add `https://cspfoundations.org/account/` as an allowed redirect URL.
3. Keep email confirmation enabled and configure a production SMTP provider before launch.
4. Create a Cloudflare Turnstile **Managed** widget for `cspfoundations.org`. Put only its public site key in `website/assets/auth-config.js`. Configure its secret directly in Supabase Authentication > Bot and Abuse Protection > CAPTCHA.
5. For GitHub, GitLab, Google, or X login, create each OAuth app and use this callback URL: `https://zaanoqtzttbbtjgbwhch.supabase.co/auth/v1/callback`. Put each client secret directly in its Supabase provider settings. The website detects disabled providers and disables their buttons.
6. Wait until GitHub Pages has issued a valid HTTPS certificate, then enable **Enforce HTTPS**. Production sign-in and sign-up intentionally refuse plain HTTP.

Accounts use Supabase Auth for password hashing and sessions. Public profiles and private identity data are stored separately with row-level security. Automated moderation can flag a case for review; it cannot ban an account from browser code.
