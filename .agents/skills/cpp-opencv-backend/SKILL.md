---
name: cpp-opencv-backend
description: Backend implementation guidance for this project's C++20/C++23 + CMake + OpenCV server. Use when Codex edits backend C++ files, CMake configuration, REST routes, WebSocket events, static React file serving, OpenCV image/video processing, job queues, logging, file handling, settings storage, security checks, or tests for backend behavior.
---

# C++ OpenCV Backend

Use this skill to keep the backend responsible for serving the React app, exposing APIs, running OpenCV jobs, managing files safely, streaming events, and enforcing LAN access security.

## Workflow

1. Read `AGENTS.md`, `TODO.md`, and the backend sections in `MasterPlan.md`.
2. Use the project-local `$cpp` skill for general modern C++ guidance such as RAII, smart pointers, STL algorithms, move semantics, concurrency, and CMake idioms.
3. When editing backend C++ behavior, consult `references/backend-cpp-practices.md` for project-specific security, ownership, OpenCV, job, and WebView2 rules.
4. Place code under the planned folders:
   - `host` for Desktop/WebView2/tray host code.
   - `server` for static files, REST, WebSocket, network info, and remote access.
   - `security` for tokens, PIN auth, sessions, and clients.
   - `image`, `video`, and `vision` for OpenCV work.
   - `jobs` for long-running processing.
   - `storage`, `logging`, and `common` for shared infrastructure.
5. Keep `backend/src/main.cpp` as a thin composition root only. It may parse args, construct classes, wire dependencies, start runtime servers, and return process status.
6. Put each durable backend responsibility in a named class with matching `.h`/`.cpp` files under its ownership folder. Do not add new backend behavior directly to `main.cpp`.
7. Keep inheritance, interface, and ownership references visible in the owning header when they are introduced, so class relationships are easy to inspect.
8. Keep route handlers thin. Put processing, security, and storage logic in services.
9. Prefer RAII value members for simple concrete services, but use heap ownership when objects are heavy, polymorphic, replaceable, or need stable lifetime. Prefer `std::unique_ptr` for exclusive heap ownership.
10. In multi-threaded owner classes, keep lock ownership in the class that owns the mutable state. Prefer `std::shared_mutex` with `std::shared_lock` for reads and `std::unique_lock` for writes when read concurrency is useful.
11. Before adding file-local utility helpers, check `backend/src/common/*Utils.*`. Shared helpers such as OpenCV color conversion, ROI/rect JSON, extension checks, filename sanitizing, UTF-8 path handling, and safe JSON extraction belong there instead of being reimplemented per `.cpp`.
12. When similar backend behavior already exists, prefer calling or extending the shared utility or owning service helper if that improves reuse without hurting readability, safety, or ownership clarity. Do not force an abstraction when a local helper is materially clearer.
13. Follow the repository root `.clang-format` for C++ source/header edits. If `clang-format` is available, format touched C++ files with `clang-format -i --style=file`.

## API And Events

- Start with `GET /api/health`, `GET /api/server-info`, and `GET /api/network-info`.
- Use consistent JSON response and error shapes.
- Emit WebSocket events for job lifecycle, logs, backend status, remote clients, previews, and pipeline node state.
- Treat the backend as authoritative for jobs, progress, results, logs, connected clients, remote access, and OpenCV state.
- Register HTTP routes in `server/ApiServer.cpp`; if route logic grows beyond request validation, service invocation, event publication, and response translation, move it into a service class.

## OpenCV And Jobs

- Keep image, video, and pipeline processing off request and UI threads.
- Use job IDs for long-running work and send progress events.
- Separate preview generation from full export.
- Keep OpenCV buffers local or cloned at service boundaries; avoid storing borrowed `cv::Mat` views beyond the owning frame lifetime.
- Use shared OpenCV utilities from `backend/src/common/OpenCvUtils.*` for repeated conversions such as grayscale/BGR normalization and ROI/rect helpers.
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
- Hide PINs, access tokens, session tokens, and sensitive local paths from non-loopback clients.
- Require an authenticated control session for non-loopback mutating requests.

## Verification

- Configure and build with CMake when backend files exist.
- Use `.\build.ps1` from the workspace root when a Release app build is requested.
- Use `scripts/prepare-debug.ps1 -SkipFrontend` or the VS Code `debug: prepare backend` task before backend debugging; the script must build and verify the exact Debug executable referenced by `launch.json`.
- Use `scripts/publish.ps1` when a distributable bundle is requested; the backend must serve static UI from the bundled `frontend/dist` without depending on the source tree.
- When migrating this skill to similar projects, preserve the root Release build, debug executable verification, publish bundle, and `/docs` guide requirements.
- Run available backend tests.
- For security-sensitive work, test unauthorized, read-only, control, and path traversal cases.
