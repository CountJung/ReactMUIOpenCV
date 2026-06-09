---
name: react-cpp-vision-platform
description: Project planning and cross-stack implementation guidance for this React + TypeScript + MUI + WebView2 + C++ OpenCV vision/media testbed. Use when Codex needs to split work from MasterPlan.md, change architecture, coordinate frontend/backend/runtime boundaries, update project TODOs, or implement features that span React UI, C++ APIs, WebSocket sync, jobs, local files, LAN access, or packaging.
---

# React CPP Vision Platform

Use this skill to keep implementation aligned with `MasterPlan.md` and the staged TODO plan. Favor small vertical slices that prove the WebView2, React, C++ server, API, and OpenCV boundaries before adding deeper feature work.

## Workflow

1. Read `MasterPlan.md`, `TODO.md`, and `AGENTS.md`.
2. Identify the phase that owns the requested work.
3. Preserve these boundaries:
   - React owns UI only.
   - Runtime adapters hide Desktop vs LAN differences.
   - C++ owns static serving, REST, WebSocket, jobs, OpenCV, local file access, and remote access state.
4. Prefer a vertical slice that includes UI, API contract, backend behavior, and verification.
5. Update `TODO.md` when a task is completed or newly discovered.

## Phase Order

Follow the masterplan priority:

- Phase 0: scaffold React/Vite/TypeScript/MUI, CMake/C++/OpenCV, static server, `/api/health`, and WebView2 host.
- Phase 1: MUI theme system and app shell.
- Phase 2: LAN Web UI Mode, QR, PIN/token auth, and remote clients.
- Phase 3: REST structure, WebSocket events, jobs, logging, and sync.
- Phase 4: Image Lab.
- Phase 5: Video Lab.
- Phase 6: React Flow pipeline and C++ `PipelineExecutor`.
- Phase 7: dashboard, charts, and data grid.
- Phase 8: packaging and template documentation.

## Architecture Checks

Before editing, check whether the change crosses one of these contracts:

- API: all major behavior is HTTP or WebSocket.
- State: jobs, progress, results, logs, clients, remote access, and OpenCV state are server-owned.
- Files: arbitrary local paths are Desktop-only; LAN clients use upload sessions or server-selected libraries.
- Security: LAN mode is off by default; external access requires PIN/token and session timeout.
- Performance: long OpenCV work runs through jobs and emits progress.

## Verification

- Verify the relevant frontend build, typecheck, or lint command when available.
- Verify CMake configure/build/tests when backend files exist.
- For cross-stack work, verify at least `/api/health`, the corresponding frontend API call, and WebSocket behavior if events are involved.
