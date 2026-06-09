# Project Agent Instructions

Read `PROJECT_MAP.md`, `MasterPlan.md`, and `TODO.md` before making architectural or feature changes.

## Project Map Discipline

- Read `PROJECT_MAP.md` before broad file exploration.
- Use `PROJECT_MAP.md` to jump to likely files before running repository-wide searches.
- Keep `PROJECT_MAP.md` fresh whenever files, folders, scripts, runtime ports, or ownership boundaries change.
- If `PROJECT_MAP.md` conflicts with the current tree, update it as part of the same change.

## Project Shape

- Build a Windows-first vision/media testbed using React, TypeScript, MUI, WebView2, C++20 or C++23, CMake, and OpenCV.
- Treat the React app as the only UI surface. WebView2 and LAN browsers must load the same UI.
- Treat the C++ backend as the owner of OpenCV processing, static file serving, REST APIs, WebSocket events, jobs, logs, local file access, and remote access state.
- Use `http://127.0.0.1:18730` as the desktop local URL unless the project explicitly changes it.

## Architecture Rules

- Do not couple React components directly to WebView2 APIs. Put runtime differences behind `frontend/src/runtime/*`.
- Put all backend calls behind typed API clients in `frontend/src/api/*`.
- Keep server-owned state authoritative for jobs, progress, results, logs, connected clients, remote access, and OpenCV processing state.
- Use WebSocket events to update or invalidate TanStack Query caches.
- Run long image/video/pipeline work through backend jobs.

## Frontend Rules

- Follow the planned `frontend/src` layout from `MasterPlan.md`.
- Use MUI theme tokens for colors, typography, shape, shadows, density, and component overrides.
- Avoid hardcoded component colors when a theme token can express the intent.
- Support desktop, tablet, and mobile layouts. Mobile should focus on status, previews, logs, and simple start/stop controls.
- Keep complex pipeline editing, large video upload, precise timeline editing, arbitrary local-path save, and system settings off mobile unless explicitly enabled later.

## Backend Rules

- Follow the planned `backend/src` layout from `MasterPlan.md`.
- Keep file-system access, path validation, upload isolation, and cleanup policies in backend-owned services.
- Default network binding to `127.0.0.1`; bind to `0.0.0.0` only when LAN Web UI Mode is explicitly enabled.
- Require PIN or temporary token auth for LAN access, apply session timeout, and default remote clients to read-only mode.
- Log codec failures, processing failures, auth failures, and remote client lifecycle events.

## Verification

- When frontend dependencies exist, run the project lint/typecheck/build commands available in `package.json`.
- When backend targets exist, configure and build with CMake, then run available tests.
- For UI changes, verify both desktop-sized and mobile-sized layouts.
- For LAN or security changes, verify binding mode, authentication, permission checks, and path traversal protections.
