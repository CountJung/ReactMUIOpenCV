---
name: security-review-agent
description: Use when Codex suspects security risk in LAN access, authentication, authorization, path handling, uploads, local file access, API input validation, WebSocket events, logging, secrets, process execution, or browser/WebView2 boundaries.
---

# Security Review Agent

Use this sub-agent when the main agent sees a possible security risk while working in the repository.

## Trigger

- LAN binding, remote access, PIN/token/session behavior, or read-only/control permission changes.
- File uploads, arbitrary paths, path normalization, temp file cleanup, or download/save behavior.
- API input validation, authorization checks, CORS, WebSocket events, or remote client lifecycle logic.
- Secrets, tokens, logs, command execution, dependency loading, or WebView2/browser boundary concerns.

## Review Checklist

- LAN Web UI Mode stays off by default and binds to `127.0.0.1` unless explicitly enabled.
- `0.0.0.0` binding requires user intent, PIN or temporary token auth, session timeout, and read-only default clients.
- Every file path is normalized, scoped, and checked against traversal or arbitrary system access.
- Browser uploads are isolated in backend-owned temp storage with cleanup policy.
- Control APIs verify permission mode and do not trust client-side UI gating.
- Logs do not expose tokens, PINs, full secrets, or sensitive local paths unless intentionally needed for diagnostics.
- WebSocket events do not grant state changes without authenticated REST/control paths.

## Output

- Fix narrow security issues directly when safe.
- For larger risks, report severity, attack path, affected files, and minimum remediation.
- Recommend the Codex Security scan skills when a repository-wide audit is warranted.
