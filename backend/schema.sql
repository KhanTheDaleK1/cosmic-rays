-- SQL Schema for Cosmic Rays Subscribers (Cloudflare D1)
-- To create the table:
-- npx wrangler d1 execute cosmic-rays-db --file=schema.sql --remote

CREATE TABLE IF NOT EXISTS subscribers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    email TEXT UNIQUE NOT NULL,
    source TEXT,
    created_at TEXT
);
