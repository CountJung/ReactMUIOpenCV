# Project Map

Keep this file fresh. Update it whenever files, folders, scripts, runtime ports, or ownership boundaries change.

Agents must read this file before broad file exploration. Use it to jump directly to likely files instead of searching the whole tree first.

## Root Files

- `MasterPlan.md`: Product and architecture masterplan.
- `TODO.md`: Phase-based execution checklist.
- `AGENTS.md`: Agent operating rules for this repository.
- `ENVIRONMENT.md`: Local development and dependency setup guide.
- `.gitignore`: Git upload exclusions for generated files, build output, dependencies, logs, runtime outputs, and env files.
- `scripts/setup-vcpkg.ps1`: Workspace-local vcpkg bootstrap script used by VSCode tasks.
- `scripts/run-backend.ps1`: Finds and runs the newest built backend executable to avoid hardcoded debug path issues.

## Frontend

- `frontend/package.json`: React/Vite/MUI dependencies and npm scripts.
- `frontend/package-lock.json`: npm dependency lockfile; commit this for reproducible installs.
- `frontend/vite.config.ts`: Vite dev server and build configuration.
- `frontend/src/main.tsx`: React entrypoint.
- `frontend/src/vite-env.d.ts`: Vite environment type declarations.
- `frontend/src/app/App.tsx`: App root.
- `frontend/src/app/providers.tsx`: Global providers such as TanStack Query and theme.
- `frontend/src/app/router.tsx`: React Router route tree.
- `frontend/src/theme/`: MUI theme tokens, component overrides, mode resolution, theme context, and theme provider.
- `frontend/src/api/`: REST and WebSocket client code. `remoteApi.ts` owns Remote Access and Network Info calls; `imageApi.ts` owns Image Lab open/upload/process/save/result calls.
- `frontend/src/runtime/`: Desktop vs LAN runtime detection and adapters. `fileAdapter.ts` hides local-path vs upload image opening.
- `frontend/src/features/`: Route-level pages for dashboard, remote access, image/video lab, pipeline, charts, data grid, logs, and settings.
- `frontend/src/shared/`: Reusable layouts, components, hooks, utilities, and shared types.

## Backend

- `backend/CMakeLists.txt`: C++ executable and library linkage.
- `backend/CMakePresets.json`: Visual Studio 2026/MSVC x64 CMake presets with workspace-local vcpkg and system-package variants.
- `backend/vcpkg.json`: C++ dependency manifest.
- `backend/src/main.cpp`: Thin composition root. Parses launch args, constructs backend services, starts WebSocket and HTTP runtimes, and reports process status.
- `backend/src/common/`: Shared constants, random IDs/PINs, ISO time formatting, API envelopes, CORS, request parsing, and loopback detection.
- `backend/src/server/ApiServer.*`: HTTP route registration, static React file serving, and thin request/response translation over backend services.
- `backend/src/server/WebSocketGateway.*`: WebSocket runtime setup, client lifecycle logging, event replay, and shutdown.
- `backend/src/server/EventHub.*`: WebSocket event envelope creation, recent event retention, and broadcast fanout.
- `backend/src/server/NetworkInfo.*`: LAN IPv4 detection and private-address sorting.
- `backend/src/security/RemoteAccessManager.*`: LAN Web UI enable/disable state, PIN/token rotation, sessions, read-only client registry, and timeout pruning.
- `backend/src/jobs/JobQueue.*`: Server-owned job records, worker thread, progress lifecycle events, cancellation, and removal.
- `backend/src/logging/LogStore.*`: spdlog setup, recent log store, and `log.appended` event publication.
- `backend/src/storage/SettingsStore.*`: Backend-owned settings state and validation.
- `backend/src/storage/PipelineStore.*`: In-memory pipeline JSON records.
- `backend/src/image/ImageResultStore.*`: Image open/upload/process/save result storage, `resultId` lookup, and preview retrieval.
- `backend/src/image/ImageFilters.*`: OpenCV image operation implementations.

## VSCode

- `.vscode/extensions.json`: Recommended extensions.
- `.vscode/settings.json`: CMake, TypeScript, ESLint, and file visibility settings.
- `.vscode/tasks.json`: Dependency install, vcpkg bootstrap, frontend dev/build/lint/typecheck/watch, backend configure/build/LAN run tasks.
- `.vscode/launch.json`: Backend, backend LAN, frontend Edge, Remote Access Edge, and compound debug configurations. Backend launch uses `${command:cmake.launchTargetPath}` to avoid stale executable paths.

## Runtime Ports

- Backend desktop URL: `http://127.0.0.1:18730`.
- Backend WebSocket URL: `ws://127.0.0.1:18731`.
- Frontend dev URL: `http://127.0.0.1:5173`.
- Frontend preview URL: `http://127.0.0.1:4173`.
- Remote Access dev URL: `http://127.0.0.1:5173/remote-access`.

## Search Hints

- App shell or navigation: start at `frontend/src/shared/layouts/AppShell.tsx` and `frontend/src/app/router.tsx`.
- Theme mode or tokens: start at `frontend/src/theme/AppThemeProvider.tsx`, `frontend/src/theme/ThemeModeContext.ts`, `frontend/src/theme/createAppTheme.ts`, and `frontend/src/theme/tokens.ts`.
- Placeholder route text: start under `frontend/src/features/*`.
- HTTP API clients: start under `frontend/src/api/*`; Image Lab calls live in `frontend/src/api/imageApi.ts`.
- Image Lab UI/runtime flow: start at `frontend/src/features/image-lab/ImageLabPage.tsx` and `frontend/src/runtime/fileAdapter.ts`.
- Remote Access UI/API: start at `frontend/src/features/remote-access/RemoteAccessPage.tsx`, `frontend/src/api/remoteApi.ts`, and `backend/src/security/RemoteAccessManager.*`.
- Backend health/static server: start at `backend/src/server/ApiServer.cpp` and `backend/src/main.cpp`.
- Backend WebSocket events: start at `backend/src/server/EventHub.*` and `backend/src/server/WebSocketGateway.*`.
- Backend jobs/logs: start at `backend/src/jobs/JobQueue.*` and `backend/src/logging/LogStore.*`.
- Backend Image Lab processing: start at `backend/src/image/ImageResultStore.*` and `backend/src/image/ImageFilters.*`.
