-- CSP Foundation authentication schema for Supabase.
-- Run this in the SQL editor for project zaanoqtzttbbtjgbwhch.
-- Copyright (c) 2025 CSP Foundation. All rights reserved.

create table if not exists public.reserved_usernames (
  username text primary key,
  reason text not null default 'Reserved by CSP Foundation',
  constraint reserved_username_normalized check (username = lower(username))
);

insert into public.reserved_usernames (username) values
  ('admin'), ('administrator'), ('moderator'), ('staff'), ('support'),
  ('root'), ('system'), ('security'), ('csp'), ('cspfoundation'),
  ('csp_foundation'), ('official')
on conflict (username) do nothing;

create table if not exists public.profiles (
  id uuid primary key references auth.users(id) on delete cascade,
  username text not null unique,
  display_name text not null,
  bio text not null default '',
  avatar_path text,
  moderation_status text not null default 'active',
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now(),
  constraint profiles_username_format check (username ~ '^[a-z0-9_]{3,30}$'),
  constraint profiles_display_name_length check (char_length(display_name) between 1 and 100),
  constraint profiles_bio_length check (char_length(bio) <= 500),
  constraint profiles_moderation_status check (moderation_status in ('active', 'limited', 'suspended'))
);

create table if not exists public.account_private (
  user_id uuid primary key references auth.users(id) on delete cascade,
  first_name text not null,
  last_name text not null,
  birth_date date not null,
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now(),
  constraint account_first_name_length check (char_length(first_name) between 1 and 80),
  constraint account_last_name_length check (char_length(last_name) between 1 and 80),
  constraint account_birth_date_range check (birth_date >= date '1900-01-01' and birth_date <= date '2100-01-01')
);

create table if not exists public.reports (
  id bigint generated always as identity primary key,
  reporter_id uuid not null references auth.users(id) on delete cascade,
  subject_user_id uuid references auth.users(id) on delete set null,
  category text not null,
  details text not null,
  status text not null default 'submitted',
  created_at timestamptz not null default now(),
  constraint reports_category check (category in ('impersonation', 'harassment', 'spam', 'unsafe_content', 'other')),
  constraint reports_details_length check (char_length(details) between 10 and 2000),
  constraint reports_status check (status in ('submitted', 'reviewing', 'resolved', 'dismissed'))
);

-- Browser clients receive no policies for these audit tables. A trusted server or
-- human moderator must review a flag before changing an account's status.
create table if not exists public.moderation_cases (
  id bigint generated always as identity primary key,
  subject_user_id uuid not null references auth.users(id) on delete cascade,
  risk_score real,
  reason text not null,
  state text not null default 'needs_review',
  reviewed_by uuid references auth.users(id) on delete set null,
  created_at timestamptz not null default now(),
  reviewed_at timestamptz,
  constraint moderation_risk_score check (risk_score is null or risk_score between 0 and 1),
  constraint moderation_state check (state in ('needs_review', 'approved', 'actioned', 'appealed', 'reversed'))
);

create table if not exists public.moderation_events (
  id bigint generated always as identity primary key,
  case_id bigint references public.moderation_cases(id) on delete set null,
  actor_id uuid references auth.users(id) on delete set null,
  action text not null,
  notes text not null default '',
  created_at timestamptz not null default now()
);

create table if not exists public.public_bans (
  user_id uuid primary key references public.profiles(id) on delete cascade,
  username text not null,
  public_reason text not null,
  banned_at timestamptz not null default now(),
  expires_at timestamptz,
  constraint public_ban_reason_length check (char_length(public_reason) between 5 and 300)
);

alter table public.moderation_cases add column if not exists content_type text;
alter table public.moderation_cases add column if not exists content_id uuid;

create table if not exists public.wiki_posts (
  id uuid primary key default gen_random_uuid(),
  author_id uuid not null references public.profiles(id) on delete cascade,
  title text not null,
  category text not null,
  body text not null,
  status text not null default 'pending',
  risk_flags text[] not null default '{}',
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now(),
  constraint wiki_title_length check (char_length(title) between 5 and 120),
  constraint wiki_body_length check (char_length(body) between 20 and 10000),
  constraint wiki_category check (category in ('question', 'tutorial', 'project', 'release', 'discussion')),
  constraint wiki_status check (status in ('pending', 'published', 'rejected'))
);

create or replace function public.screen_wiki_post()
returns trigger
language plpgsql
security definer
set search_path = ''
as $$
declare
  content text := lower(new.title || ' ' || new.body);
  flags text[] := '{}';
  recent_count integer;
begin
  if not exists (
    select 1 from public.profiles p
    where p.id = new.author_id and p.moderation_status = 'active'
  ) then
    raise exception 'This account cannot publish posts';
  end if;

  select count(*) into recent_count from public.wiki_posts p
  where p.author_id = new.author_id and p.created_at > now() - interval '1 hour';
  if recent_count >= 5 then
    raise exception 'Post rate limit reached; try again later';
  end if;

  if content ~ '\m(hack|hacking|exploit|exploiting|malware|ransomware|ddos|credential theft|steal passwords)\M' then
    flags := array_append(flags, 'security_or_exploitation');
  end if;
  if content ~ '\m(idiot|moron|stupid|hate|abuse|harass|kill you)\M' then
    flags := array_append(flags, 'abuse_or_disrespect');
  end if;
  if content ~ '\m(blame|accuse|fraud|criminal|scammer)\M' then
    flags := array_append(flags, 'accusation_or_blame');
  end if;

  new.risk_flags := flags;
  new.status := case when cardinality(flags) = 0 then 'published' else 'pending' end;
  if cardinality(flags) > 0 then
    insert into public.moderation_cases (subject_user_id, risk_score, reason, content_type, content_id)
    values (new.author_id, 0.75, 'Wiki screening: ' || array_to_string(flags, ', '), 'wiki_post', new.id);
  end if;
  return new;
end;
$$;

create or replace function public.apply_moderation_action(target_user uuid, new_status text, reason text)
returns void
language plpgsql
security definer
set search_path = ''
as $$
declare
  target_username text;
begin
  if new_status not in ('active', 'limited', 'suspended') then
    raise exception 'Invalid moderation status';
  end if;
  if char_length(trim(reason)) not between 5 and 300 then
    raise exception 'A public reason between 5 and 300 characters is required';
  end if;
  update public.profiles set moderation_status = new_status where id = target_user
  returning username into target_username;
  if target_username is null then raise exception 'Account not found'; end if;
  if new_status = 'suspended' then
    insert into public.public_bans (user_id, username, public_reason)
    values (target_user, target_username, trim(reason))
    on conflict (user_id) do update set username = excluded.username, public_reason = excluded.public_reason, banned_at = now();
  else
    delete from public.public_bans where user_id = target_user;
  end if;
  insert into public.moderation_events (actor_id, action, notes)
  values (auth.uid(), 'account_' || new_status, trim(reason));
end;
$$;

revoke all on function public.apply_moderation_action(uuid, text, text) from public, anon, authenticated;
grant execute on function public.apply_moderation_action(uuid, text, text) to service_role;

create or replace function public.username_available(candidate text)
returns boolean
language sql
stable
security definer
set search_path = ''
as $$
  select
    lower(trim(candidate)) ~ '^[a-z0-9_]{3,30}$'
    and not exists (
      select 1 from public.reserved_usernames r
      where r.username = lower(trim(candidate))
    )
    and not exists (
      select 1 from public.profiles p
      where p.username = lower(trim(candidate))
    );
$$;

create or replace function public.set_updated_at()
returns trigger
language plpgsql
set search_path = ''
as $$
begin
  new.updated_at = now();
  return new;
end;
$$;

create or replace function public.validate_private_account()
returns trigger
language plpgsql
set search_path = ''
as $$
begin
  if new.birth_date > current_date then
    raise exception 'Birth date cannot be in the future';
  end if;
  return new;
end;
$$;

create or replace function public.validate_public_profile()
returns trigger
language plpgsql
security definer
set search_path = ''
as $$
begin
  new.username := lower(trim(new.username));
  if new.username !~ '^[a-z0-9_]{3,30}$'
     or exists (select 1 from public.reserved_usernames r where r.username = new.username)
     or exists (select 1 from public.profiles p where p.username = new.username and p.id <> new.id) then
    raise exception 'Username is invalid, reserved, or already in use';
  end if;
  return new;
end;
$$;

create or replace function public.handle_new_user()
returns trigger
language plpgsql
security definer
set search_path = ''
as $$
declare
  requested_username text := lower(trim(coalesce(new.raw_user_meta_data ->> 'username', '')));
  chosen_username text;
  chosen_display_name text;
  given_name text := trim(coalesce(new.raw_user_meta_data ->> 'first_name', new.raw_user_meta_data ->> 'given_name', ''));
  family_name text := trim(coalesce(new.raw_user_meta_data ->> 'last_name', new.raw_user_meta_data ->> 'family_name', ''));
  supplied_birth_date text := nullif(trim(coalesce(new.raw_user_meta_data ->> 'birth_date', '')), '');
begin
  if requested_username = '' then
    chosen_username := 'user_' || substr(replace(new.id::text, '-', ''), 1, 12);
  elsif not public.username_available(requested_username) then
    raise exception 'Username is invalid, reserved, or already in use';
  else
    chosen_username := requested_username;
  end if;

  chosen_display_name := left(nullif(trim(coalesce(
    new.raw_user_meta_data ->> 'display_name',
    new.raw_user_meta_data ->> 'full_name',
    concat_ws(' ', given_name, family_name)
  )), ''), 100);
  if chosen_display_name is null then chosen_display_name := chosen_username; end if;

  insert into public.profiles (id, username, display_name, bio)
  values (new.id, chosen_username, chosen_display_name, left(coalesce(new.raw_user_meta_data ->> 'bio', ''), 500));

  -- OAuth providers may omit legal name and birth date; those fields stay absent.
  if given_name <> '' and family_name <> '' and supplied_birth_date is not null then
    insert into public.account_private (user_id, first_name, last_name, birth_date)
    values (new.id, left(given_name, 80), left(family_name, 80), supplied_birth_date::date);
  end if;
  return new;
end;
$$;

drop trigger if exists profiles_set_updated_at on public.profiles;
create trigger profiles_set_updated_at before update on public.profiles
for each row execute function public.set_updated_at();

drop trigger if exists profiles_validate on public.profiles;
create trigger profiles_validate before insert or update on public.profiles
for each row execute function public.validate_public_profile();

drop trigger if exists account_private_set_updated_at on public.account_private;
create trigger account_private_set_updated_at before update on public.account_private
for each row execute function public.set_updated_at();

drop trigger if exists account_private_validate on public.account_private;
create trigger account_private_validate before insert or update on public.account_private
for each row execute function public.validate_private_account();

drop trigger if exists wiki_posts_set_updated_at on public.wiki_posts;
create trigger wiki_posts_set_updated_at before update on public.wiki_posts
for each row execute function public.set_updated_at();

drop trigger if exists wiki_posts_screen on public.wiki_posts;
create trigger wiki_posts_screen before insert on public.wiki_posts
for each row execute function public.screen_wiki_post();

drop trigger if exists on_auth_user_created on auth.users;
create trigger on_auth_user_created after insert on auth.users
for each row execute function public.handle_new_user();

-- Give pre-existing Auth users a safe generated profile.
insert into public.profiles (id, username, display_name)
select u.id,
       'user_' || substr(replace(u.id::text, '-', ''), 1, 12),
       coalesce(nullif(left(trim(coalesce(u.raw_user_meta_data ->> 'full_name', '')), 100), ''), 'CSP member')
from auth.users u
where not exists (select 1 from public.profiles p where p.id = u.id)
on conflict do nothing;

alter table public.reserved_usernames enable row level security;
alter table public.profiles enable row level security;
alter table public.account_private enable row level security;
alter table public.reports enable row level security;
alter table public.moderation_cases enable row level security;
alter table public.moderation_events enable row level security;
alter table public.public_bans enable row level security;
alter table public.wiki_posts enable row level security;

drop policy if exists "Public active profiles are readable" on public.profiles;
create policy "Public active profiles are readable" on public.profiles for select
using (moderation_status = 'active' or id = auth.uid());

drop policy if exists "Owners update public profile fields" on public.profiles;
create policy "Owners update public profile fields" on public.profiles for update
using (id = auth.uid() and moderation_status = 'active')
with check (id = auth.uid() and moderation_status = 'active');

drop policy if exists "Owners read private account data" on public.account_private;
create policy "Owners read private account data" on public.account_private for select
using (user_id = auth.uid());

drop policy if exists "Owners update private account data" on public.account_private;
create policy "Owners update private account data" on public.account_private for update
using (user_id = auth.uid()) with check (user_id = auth.uid());

drop policy if exists "Users submit reports" on public.reports;
create policy "Users submit reports" on public.reports for insert
with check (reporter_id = auth.uid());

drop policy if exists "Users read their reports" on public.reports;
create policy "Users read their reports" on public.reports for select
using (reporter_id = auth.uid());

drop policy if exists "Published and owned wiki posts are readable" on public.wiki_posts;
create policy "Published and owned wiki posts are readable" on public.wiki_posts for select
using (status = 'published' or author_id = auth.uid());

drop policy if exists "Active users submit wiki posts" on public.wiki_posts;
create policy "Active users submit wiki posts" on public.wiki_posts for insert
with check (
  author_id = auth.uid()
  and exists (select 1 from public.profiles p where p.id = auth.uid() and p.moderation_status = 'active')
);

drop policy if exists "Public ban records are readable" on public.public_bans;
create policy "Public ban records are readable" on public.public_bans for select
using (true);

revoke all on table public.reserved_usernames, public.profiles, public.account_private,
  public.reports, public.moderation_cases, public.moderation_events from anon, authenticated;
grant select on table public.profiles to anon, authenticated;
grant update (username, display_name, bio, avatar_path, updated_at) on public.profiles to authenticated;
grant select on table public.account_private to authenticated;
grant update (first_name, last_name, birth_date, updated_at) on public.account_private to authenticated;
grant select, insert on table public.reports to authenticated;
grant execute on function public.username_available(text) to anon, authenticated;
revoke all on table public.public_bans from anon, authenticated;
grant select (username, public_reason, banned_at, expires_at) on public.public_bans to anon, authenticated;
revoke all on table public.wiki_posts from anon, authenticated;
grant select on table public.wiki_posts to anon, authenticated;
grant insert (author_id, title, category, body) on public.wiki_posts to authenticated;

insert into storage.buckets (id, name, public, file_size_limit, allowed_mime_types)
values ('avatars', 'avatars', true, 2097152, array['image/jpeg', 'image/png', 'image/webp'])
on conflict (id) do update set
  public = excluded.public,
  file_size_limit = excluded.file_size_limit,
  allowed_mime_types = excluded.allowed_mime_types;

drop policy if exists "Public avatars are readable" on storage.objects;
create policy "Public avatars are readable" on storage.objects for select
using (bucket_id = 'avatars');

drop policy if exists "Users upload their avatar" on storage.objects;
create policy "Users upload their avatar" on storage.objects for insert to authenticated
with check (bucket_id = 'avatars' and (storage.foldername(name))[1] = auth.uid()::text);

drop policy if exists "Users update their avatar" on storage.objects;
create policy "Users update their avatar" on storage.objects for update to authenticated
using (bucket_id = 'avatars' and (storage.foldername(name))[1] = auth.uid()::text)
with check (bucket_id = 'avatars' and (storage.foldername(name))[1] = auth.uid()::text);

drop policy if exists "Users delete their avatar" on storage.objects;
create policy "Users delete their avatar" on storage.objects for delete to authenticated
using (bucket_id = 'avatars' and (storage.foldername(name))[1] = auth.uid()::text);
