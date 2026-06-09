# Project Contract

- Desktop URL: `http://127.0.0.1:18730`.
- LAN binding: off by default; explicit enable switches to `0.0.0.0`.
- Frontend: React, TypeScript, MUI, React Router, TanStack Query, React Flow.
- Backend: C++20 or C++23, CMake, OpenCV, REST API, WebSocket server, jobs, logs, storage.
- Core rule: React must not directly depend on WebView2-only APIs.
- Core rule: Local file paths are Desktop-only.
- Core rule: Long processing must run through jobs and progress events.
