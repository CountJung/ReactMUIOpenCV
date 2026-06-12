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
- Maintain both runtime modes when this project type needs them: desktop app mode through WebView2 and web/server mode through browser access to the local backend.
- Treat the C++ backend as the owner of OpenCV processing, static file serving, REST APIs, WebSocket events, jobs, logs, local file access, and remote access state.
- Use `http://127.0.0.1:18730` as the desktop local URL unless the project explicitly changes it.

## Architecture Rules

- Do not couple React components directly to WebView2 APIs. Put runtime differences behind `frontend/src/runtime/*`.
- Put all backend calls behind typed API clients in `frontend/src/api/*`.
- Keep server-owned state authoritative for jobs, progress, results, logs, connected clients, remote access, and OpenCV processing state.
- Use WebSocket events to update or invalidate TanStack Query caches.
- Run long image/video/pipeline work through backend jobs.
- Prefer value members for simple RAII state, but use heap ownership when objects are heavy, polymorphic, replaceable, or need stable lifetime across composition boundaries. Prefer `std::unique_ptr` for exclusive heap ownership.
- In multi-threaded backend services, the class that owns mutable state owns the lock protecting that state. Use shared/read locks for const reads and unique/write locks for mutations when read concurrency is useful.

## Sub-Agent Delegation Rules

When the main agent discovers one of the following conditions while working, delegate the focused review or fix to the matching project sub-agent. Keep the delegated task narrow, provide the relevant files and symptoms, and do not use sub-agents to expand the requested scope without a concrete trigger.

- Use `$code-review-agent` when warnings, errors, failing checks, brittle code, or regressions are found outside the immediate work scope.
- Use `$memory-leak-review-agent` when React cleanup, async cancellation, media buffer lifetime, C++ ownership, RAII, thread shutdown, socket shutdown, file handle cleanup, OpenCV resource lifetime, or WebView2 resource lifetime looks suspicious.
- Use `$security-review-agent` when LAN access, authentication, authorization, path handling, uploads, local file access, API validation, WebSocket events, logging, secrets, command execution, or browser/WebView2 boundaries look risky.
- Use `$project-structure-review-agent` when any single source file exceeds 1000 lines or a file is accumulating responsibilities that should be split according to the project architecture.

## Build, Debug, And Publish Rules

- For this project type, VS Code debugging must prepare the program so it can run immediately: check frontend dependencies, typecheck frontend code, bootstrap backend dependencies, configure CMake, build the Debug executable, verify the executable path, and then enter the debugger.
- When both app and web modes exist, provide separate debug entries for desktop app mode and web/server mode. The app host must load the same local UI URL as browser web mode.
- Do not rely on editor-global or manually selected launch targets when a stable Debug executable path is available. Prelaunch tasks must create and verify the exact executable used by `launch.json`.
- The Release build implementation must live under `scripts\build.ps1`. Running `.\build.ps1` from the workspace root must remain supported as a thin compatibility wrapper that delegates to `scripts\build.ps1`, prepares frontend dependencies, builds the production frontend bundle, configures the backend, builds the Release executable, and fails clearly if the expected executable is missing.
- If external distribution is requested, publish artifacts under `/publish` and write user-facing guide documents under `/docs`.
- Publish bundles for this project type should include app-mode and web-mode launch scripts when both modes are supported.
- When migrating AGENTS.md, project skills, or template instructions to another similar React + C++ desktop/web project, migrate these build/debug/publish rules with them.

## Frontend Rules

- Follow the planned `frontend/src` layout from `MasterPlan.md`.
- Use MUI theme tokens for colors, typography, shape, shadows, density, and component overrides.
- Avoid hardcoded component colors when a theme token can express the intent.
- Support desktop, tablet, and mobile layouts. Mobile should focus on status, previews, logs, and simple start/stop controls.
- Keep complex pipeline editing, large video upload, precise timeline editing, arbitrary local-path save, and system settings off mobile unless explicitly enabled later.

## Backend Rules

- Follow the planned `backend/src` layout from `MasterPlan.md`.
- Keep `backend/src/main.cpp` as a thin composition root only: parse process arguments, construct services, wire dependencies, start/stop runtime servers, and return process status.
- Do not implement REST route logic, OpenCV processing, logging stores, job queues, remote access state, or file handling directly in `main.cpp`.
- Put each durable backend responsibility in a named class with matching `.h`/`.cpp` files under its ownership folder. Keep inheritance or interface relationships visible in the same header when they are introduced so references are easy to inspect.
- Keep route handlers thin in `server/ApiServer.cpp`; route handlers should validate request shape, call service classes, publish events, and translate errors into the shared API envelope.
- Keep file-system access, path validation, upload isolation, and cleanup policies in backend-owned services.
- Keep ownership and synchronization visible in backend headers: use RAII values for light local resources, `std::unique_ptr` for exclusive heap-owned services or heavy replaceable components, and `std::shared_mutex` with `std::shared_lock`/`std::unique_lock` for owner-managed read/write state where concurrent reads are expected.
- Default network binding to `127.0.0.1`; bind to `0.0.0.0` only when LAN Web UI Mode is explicitly enabled.
- Require PIN or temporary token auth for LAN access, apply session timeout, and default remote clients to read-only mode.
- Log codec failures, processing failures, auth failures, and remote client lifecycle events.

## Verification

- When frontend dependencies exist, run the project lint/typecheck/build commands available in `package.json`.
- When backend targets exist, configure and build with CMake, then run available tests.
- For debug workflow changes, run the debug preparation script and verify the exact executable referenced by VS Code exists.
- For Release build changes, run `scripts\build.ps1` and keep the compatibility wrapper `.\build.ps1` working.
- For publish workflow changes, run `scripts/publish.ps1` and verify the generated bundle can serve `/api/health` and the React UI.
- For UI changes, verify both desktop-sized and mobile-sized layouts.
- When the Browser plugin cannot expose an in-app browser through `agent.browsers.get("iab")`, use the `mcp__playwright` browser tools as the internal-browser fallback for localhost/UI verification before falling back to an external browser.
- For LAN or security changes, verify binding mode, authentication, permission checks, and path traversal protections.
