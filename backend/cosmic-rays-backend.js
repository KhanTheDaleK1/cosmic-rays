/**
 * Cloudflare Worker for Cosmic Rays Backend
 * Features:
 * 1. Store emails in D1 Database
 * 2. Send notification to Discord/Slack on new download
 * 3. Protected endpoint to list all subscribers
 */

export default {
  async fetch(request, env) {
    const url = new URL(request.url);

    // CORS Headers
    const corsHeaders = {
      "Access-Control-Allow-Origin": "*",
      "Access-Control-Allow-Methods": "POST, GET, OPTIONS",
      "Access-Control-Allow-Headers": "Content-Type, Authorization",
    };

    if (request.method === "OPTIONS") {
      return new Response(null, { headers: corsHeaders });
    }

    // --- 1. SUBSCRIPTION ENDPOINT (POST /subscribe) ---
    if (url.pathname === "/subscribe" && request.method === "POST") {
      try {
        const { email, source } = await request.json();

        if (!email || !email.includes("@")) {
          return new Response("Invalid email", { status: 400, headers: corsHeaders });
        }

        // Store in D1
        const { success, meta } = await env.DB.prepare(
          "INSERT OR IGNORE INTO subscribers (email, source, created_at) VALUES (?, ?, ?)"
        )
          .bind(email, source || "cosmic-rays", new Date().toISOString())
          .run();

        // Send Notification if it's a NEW subscriber
        if (success && meta.changes > 0 && env.DISCORD_WEBHOOK_URL) {
          await fetch(env.DISCORD_WEBHOOK_URL, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
              content: `🚀 **New Subscriber!**\n**Email:** \`${email}\`\n**Date:** ${new Date().toLocaleString()}`,
            }),
          });
        }

        return new Response(JSON.stringify({ success: true }), {
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        });
      } catch (err) {
        return new Response(err.message, { status: 500, headers: corsHeaders });
      }
    }

    // --- 2. LIST EMAILS ENDPOINT (GET /emails) ---
    if (url.pathname === "/emails" && request.method === "GET") {
      // Simple Auth: Check for API Key in header
      const authHeader = request.headers.get("Authorization");
      if (!authHeader || authHeader !== `Bearer ${env.ADMIN_API_KEY}`) {
        return new Response("Unauthorized", { status: 401, headers: corsHeaders });
      }

      try {
        const { results } = await env.DB.prepare("SELECT * FROM subscribers ORDER BY created_at DESC").all();
        return new Response(JSON.stringify(results), {
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        });
      } catch (err) {
        return new Response(err.message, { status: 500, headers: corsHeaders });
      }
    }

    return new Response("Not Found", { status: 404, headers: corsHeaders });
  },
};

/**
 * DATABASE SCHEMA (D1)
 * 
 * CREATE TABLE subscribers (
 *   id INTEGER PRIMARY KEY AUTOINCREMENT,
 *   email TEXT UNIQUE NOT NULL,
 *   source TEXT,
 *   created_at TEXT
 * );
 */
