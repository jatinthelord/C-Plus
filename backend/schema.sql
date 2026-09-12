-- CSP Foundation catalog. Load after creating this schema:
--   sqlite3 website/data/csp.db < backend/schema.sql
--   sqlite3 website/data/csp.db < backend/seed.sql

PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS packages (
  name TEXT PRIMARY KEY,
  version TEXT NOT NULL,
  description TEXT NOT NULL,
  repository TEXT NOT NULL,
  arch TEXT NOT NULL,
  license TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS libraries (
  path TEXT PRIMARY KEY,
  name TEXT NOT NULL,
  bytes INTEGER NOT NULL,
  sha256 TEXT NOT NULL,
  summary TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS sources (
  file TEXT PRIMARY KEY,
  bytes INTEGER NOT NULL,
  sha256 TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS oauth_sessions (
  id TEXT PRIMARY KEY,
  provider TEXT NOT NULL,
  subject TEXT NOT NULL,
  login TEXT NOT NULL,
  website_notified INTEGER NOT NULL DEFAULT 0,
  created_at TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS packages_repo ON packages(repository);
CREATE INDEX IF NOT EXISTS sources_bytes ON sources(bytes);
