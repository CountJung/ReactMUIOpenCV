---
name: cpp-opencv-backend
description: Backend implementation guidance for this project's C++20/C++23 + CMake + OpenCV server. Use when Codex edits backend C++ files, CMake configuration, REST routes, WebSocket events, static React file serving, OpenCV image/video processing, job queues, logging, file handling, settings storage, security checks, or tests for backend behavior.
---

# C++ OpenCV Backend

Use this skill to keep the backend responsible for serving the React app, exposing APIs, running OpenCV jobs, managing files safely, streaming events, and enforcing LAN access security.

## Workflow

1. Read `AGENTS.md`, `TODO.md`, and the backend sections in `MasterPlan.md`.
2. Place code under the planned folders:
   - `host` for Desktop/WebView2/tray host code.
   - `server` for static files, REST, WebSocket, network info, and remote access.
   - `security` for tokens, PIN auth, sessions, and clients.
   - `image`, `video`, and `vision` for OpenCV work.
   - `jobs` for long-running processing.
   - `storage`, `logging`, and `common` for shared infrastructure.
3. Keep route handlers thin. Put processing, security, and storage logic in services.

## API And Events

- Start with `GET /api/health`, `GET /api/server-info`, and `GET /api/network-info`.
- Use consistent JSON response and error shapes.
- Emit WebSocket events for job lifecycle, logs, backend status, remote clients, previews, and pipeline node state.
- Treat the backend as authoritative for jobs, progress, results, logs, connected clients, remote access, and OpenCV state.

## OpenCV And Jobs

- Keep image, video, and pipeline processing off request and UI threads.
- Use job IDs for long-running work and send progress events.
- Separate preview generation from full export.
- Log codec, file, parameter, and processing failures with enough context to debug.
- Add cleanup policies for temp files and result files.

## File And Security Rules

- Keep arbitrary local-path access Desktop-only.
- Isolate browser uploads in a temp directory.
- Normalize and validate paths to prevent traversal.
- Default LAN access to off and bind to `127.0.0.1`.
- Bind to `0.0.0.0` only after explicit user enable.
- Require PIN or temporary token auth for LAN clients and apply session timeout.
- Default remote clients to read-only mode.

## Verification

- Configure and build with CMake when backend files exist.
- Run available backend tests.
- For security-sensitive work, test unauthorized, read-only, control, and path traversal cases.
