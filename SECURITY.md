# Security Policy

## Supported Versions

We actively provide security updates for the following versions:

| Version | Supported          |
| ------- | ------------------ |
| Latest  | :white_check_mark: |
| Beta    | :white_check_mark: |
| Legacy  | :x:                |

## Reporting a Vulnerability

We take the security of **Cosmic Rays** and our users' systems very seriously. If you discover a security vulnerability, we would appreciate it if you could report it to us in a responsible manner.

### How to Report
Please **do not** report security vulnerabilities via public GitHub issues. Instead, send an email to:
**security@cosmic.audio** (or use the feedback mechanism on the official website).

Please include the following in your report:
- A detailed description of the vulnerability.
- Steps to reproduce the issue (proof-of-concept).
- Your operating system and the version of Cosmic Rays you are using.

### Our Response Process
1. **Acknowledgement:** We will acknowledge receipt of your report within 48 hours.
2. **Investigation:** We will investigate the issue and determine its severity based on how it impacts the real-time audio thread or the cloud-based backend.
3. **Resolution:** If a vulnerability is confirmed, we will aim to provide a fix in the next automated build cycle.
4. **Disclosure:** Once a fix is deployed, we will coordinate with you to disclose the vulnerability if necessary.

## Scope of Security

### 1. Real-Time Processing
The core plugin operates with high-level system privileges to ensure low-latency audio. We rigorously audit our DSP algorithms to prevent buffer overflows, memory leaks, and out-of-bounds memory access that could lead to local privilege escalation.

### 2. Cloud Integration (Backend)
The `backend/` components (Cloudflare Workers, D1 Database) are used for beta-user subscriptions and update notifications. We follow best practices for:
- **API Key Protection:** Secrets are stored securely in GitHub Secrets and Cloudflare Environment Variables.
- **Data Privacy:** We only collect minimal user data (email) and never store passwords or sensitive financial information.

### 3. CI/CD Pipeline
Our automated builds run on GitHub-hosted runners. Every build is subject to **OpenSSF security audits** and dependency scanning via Renovate to prevent supply-chain attacks.

---
*Thank you for helping keep the granular void secure!*
