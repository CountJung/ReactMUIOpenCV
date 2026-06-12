# Project Map

Keep this file fresh. Update it whenever files, folders, scripts, runtime ports, or ownership boundaries change.

Agents must read this file before broad file exploration. Use it to jump directly to likely files instead of searching the whole tree first.

## Root Files

- `MasterPlan.md`: Product and architecture masterplan.
- `TODO.md`: Phase-based execution checklist.
- `AGENTS.md`: Agent operating rules for this repository.
- `.agents/skills/`: Project-local Codex skills, including implementation guidance, C++ backend reference practices, and sub-agent review roles for code warnings/errors, memory/resource leaks, security risk, and oversized source structure.
- `.agents/skills/cpp/`: Project-installed copy of the root `reference/cpp-1.0.0` general C++ skill, including modern C++ and CMake templates.
- `.agents/skills/cpp-opencv-backend/references/backend-cpp-practices.md`: Project-specific C++ backend practices for ownership boundaries, security defaults, OpenCV resources, WebView2 lifecycle, jobs/events, and structure thresholds.
- `reference/`: Local reference skill packages staged before project installation; currently contains the source `cpp-1.0.0` skill and archive.
- `ENVIRONMENT.md`: Local development and dependency setup guide.
- `.gitignore`: Git upload exclusions for generated files, build output, dependencies, logs, runtime outputs, and env files.
- `README.md`: Fresh Windows PC quick start and script-driven setup guide.
- `build.ps1`: Compatibility wrapper that delegates to `scripts/build.ps1` so legacy root build commands still work.
- `docs/`: Korean user, developer setup, publishing, build/debug policy, and coding guide documentation.
- `scripts/bootstrap.ps1`: Fresh Windows PC bootstrap entrypoint. Optionally installs Windows prerequisites, prepares WebView2 SDK, frontend dependencies, vcpkg, and runs the Release build.
- `scripts/configure-backend.ps1`: Configures the backend build directory after selecting the newest supported installed Visual Studio CMake generator.
- `scripts/build-backend.ps1`: Builds the backend Debug or Release configuration using the configured backend build directory.
- `scripts/install-windows-prereqs.ps1`: winget-based installer for Node.js LTS, CMake, Visual Studio 2022 Build Tools, and WebView2 Runtime.
- `scripts/script-utils.ps1`: Shared PowerShell helpers for checked native command execution, installed Visual Studio generator selection, backend configure/build, and generated CMake cache cleanup when the configured generator changes.
- `scripts/setup-webview2-sdk.ps1`: Downloads and extracts the WebView2 SDK NuGet package to `%USERPROFILE%\.nuget\packages`.
- `scripts/ensure-frontend-deps.ps1`: Conditionally runs `npm install` when frontend dependencies are missing or stale.
- `scripts/prepare-debug.ps1`: VS Code debug preparation script for dependency checks, optional workspace runtime shutdown, frontend typecheck or static bundle build, CMake configure, and Debug backend build.
- `scripts/build.ps1`: Release build script. Ensures frontend dependencies, builds `frontend/dist`, configures CMake, and builds the Release backend executable plus WebView2 desktop app host when the SDK is available.
- `scripts/publish.ps1`: Creates `/publish/ReactMUIOpenCV`, versioned zip, and `ReactMUIOpenCV-latest.zip` from Release outputs.
- `scripts/setup-vcpkg.ps1`: Workspace-local vcpkg bootstrap script used by VSCode tasks.
- `scripts/start.ps1`: Starts Debug or Release outputs in desktop app, web browser, or LAN mode.
- `scripts/run-backend.ps1`: Finds and runs the newest built backend executable to avoid hardcoded debug path issues.
- `data/pipelines.json`: Backend-owned persisted pipeline documents and recent pipeline execution summaries. Created at runtime when pipelines are saved.
- `data/video-diagnostics.json`: Backend-owned persisted Video Lab FPS/read diagnostics and motion/stabilization metrics. Created at runtime when Measure FPS or Analyze Motion is executed.
- `data/video-tracking.json`: Backend-owned persisted object-tracking summaries and per-frame ROI metadata. Created at runtime when Track ROI or a `trackObject` pipeline node is executed.

## Documentation

- `docs/USER_GUIDE.md`: Korean end-user guide for desktop app mode, web mode, LAN mode, installation, and troubleshooting.
- `docs/DEVELOPER_SETUP_GUIDE.md`: Korean step-by-step developer onboarding guide for first setup, VS Code debug preparation, React/C++ resource linking, Release build, and publish bundle creation.
- `docs/PIPELINE_SCHEMA.md`: Shared Phase 6 pipeline document schema used by React Flow and the C++ `PipelineExecutor`.
- `docs/PUBLISHING.md`: Korean guide for creating `/publish` bundles and uploading zip artifacts to an external web server.
- `docs/BUILD_AND_DEBUG_POLICY.md`: Korean policy for root Release builds, VS Code debug readiness, executable verification, and publish requirements.
- `docs/CODING_GUIDE.md`: Korean coding guide for human contributors covering backend structure, frontend structure, API/event rules, UI rules, and verification.

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
- `frontend/src/api/`: REST and WebSocket client code. `remoteApi.ts` owns Remote Access and Network Info calls; `imageApi.ts` owns Image Lab open/upload/process/save/result calls; `videoApi.ts` owns Video Lab open/upload/frame/extract/export/motion metrics/tracking calls; `pipelineApi.ts` owns Phase 6 pipeline document, CRUD, execution, and execution result types including image/video/tracking nodes; `jobsApi.ts`, `logsApi.ts`, and `filesApi.ts` feed Dashboard, Charts, Logs, and Data Grid state.
- `frontend/src/runtime/`: Desktop vs LAN runtime detection and adapters. `fileAdapter.ts` hides local-path vs upload image/video opening.
- `frontend/src/features/`: Route-level pages for dashboard, remote access, image/video lab, pipeline, charts, data grid, logs, and settings.
- `frontend/src/shared/`: Reusable layouts, components, hooks, utilities, and shared types.

## Backend

- `backend/CMakeLists.txt`: C++ executable and library linkage, warning options, and CMake compile command export request for generators that support it.
- `backend/CMakePresets.json`: Manual Visual Studio 2022/MSVC x64 CMake presets with workspace-local vcpkg and system-package variants. Project scripts auto-select the newest supported installed Visual Studio generator while preserving the same `backend/out/build/windows-msvc-vcpkg` output path.
- `backend/vcpkg.json`: C++ dependency manifest.
- `backend/src/main.cpp`: Thin composition root. Parses launch args, constructs backend services, starts WebSocket and HTTP runtimes, and reports process status.
- `backend/src/app/AppContext.*`: Backend runtime context. Builds runtime config, owns service lifetimes, wires dependencies, starts WebSocket and HTTP runtimes, and preserves shutdown order.
- `backend/src/host/WebViewHost.cpp`: Win32/WebView2 desktop app host. Checks WebView2 Runtime availability, starts the local backend server when needed, remembers window placement, and loads `http://127.0.0.1:18730` in an app window.
- `backend/src/common/`: Shared constants, random IDs/PINs, ISO time formatting, API envelopes, CORS, request parsing, and loopback detection.
- `backend/src/server/ApiServer.*`: HTTP route registration, static React file serving from a resolved source-tree or bundled `frontend/dist`, and thin request/response translation over backend services.
- `backend/src/server/WebSocketGateway.*`: WebSocket runtime setup, client lifecycle logging, event replay, and shutdown.
- `backend/src/server/EventHub.*`: WebSocket event envelope creation, recent event retention, and broadcast fanout.
- `backend/src/server/NetworkInfo.*`: LAN IPv4 detection and private-address sorting.
- `backend/src/security/RemoteAccessManager.*`: LAN Web UI enable/disable state, PIN/token rotation, sessions, read-only client registry, and timeout pruning.
- `backend/src/jobs/JobQueue.*`: Server-owned job records, worker thread, progress lifecycle events, cancellation, and removal.
- `backend/src/logging/LogStore.*`: spdlog setup, recent log store, and `log.appended` event publication.
- `backend/src/storage/SettingsStore.*`: Backend-owned settings state and validation.
- `backend/src/storage/PipelineStore.*`: Backend-owned pipeline JSON records persisted to `data/pipelines.json`, including recent execution summaries and storage location metadata for loopback clients.
- `backend/src/storage/VideoDiagnosticsStore.*`: Backend-owned Video Lab diagnostics records persisted to `data/video-diagnostics.json`, including measured read FPS, metadata FPS, sample frames, optical-flow/stabilization metrics, elapsed time, and storage location metadata for loopback clients.
- `backend/src/storage/VideoTrackingStore.*`: Backend-owned object tracking records persisted to `data/video-tracking.json`, including ROI, per-frame bounding boxes, match scores, status, and storage location metadata for loopback clients. Uses owner-managed `std::shared_mutex` with shared reads and unique writes.
- `backend/src/image/ImageResultStore.*`: Image open/upload/process/save result storage, `resultId` lookup, and preview retrieval.
- `backend/src/image/ImageFilters.*`: OpenCV image operation implementations.
- `backend/src/video/VideoService.*`: Video open/upload metadata extraction, preview frame reading, frame extraction, filter preview, optical-flow overlay, LK feature translation stabilization, template-match ROI tracking, motion metrics, and MJPG export.
- `backend/src/vision/PipelineExecutor.*`: Phase 6 C++ image/video pipeline execution over React Flow JSON, Image Lab result IDs, Video Lab video IDs, OpenCV operation nodes, node events, and cached intermediate results, motion metrics, or tracking metadata.

## VSCode

- `.vscode/extensions.json`: Recommended extensions.
- `.vscode/settings.json`: CMake, TypeScript, ESLint, and file visibility settings.
- `.vscode/tasks.json`: Conditional dependency checks, fresh bootstrap, vcpkg bootstrap, debug preparation, frontend dev/build/lint/typecheck/watch, backend Debug/Release configure/build/LAN run tasks, Release build task, and publish task.
- `.vscode/launch.json`: Desktop app, backend, backend LAN, frontend Edge, Remote Access Edge, and compound debug configurations. App/backend launch uses explicit Debug executable paths created by `debug: prepare backend`; frontend launch starts `frontend: dev`, which prepares dependencies and typechecks first.

## Runtime Ports

- Backend desktop URL: `http://127.0.0.1:18730`.
- Backend WebSocket URL: `ws://127.0.0.1:18731`.
- Desktop app host: `backend/out/build/windows-msvc-vcpkg/{Debug|Release}/ReactMUIOpenCVApp.exe`.
- Web/server mode executable: `backend/out/build/windows-msvc-vcpkg/{Debug|Release}/ReactMUIOpenCV.exe`.
- Frontend dev URL: `http://127.0.0.1:5173`.
- Frontend preview URL: `http://127.0.0.1:4173`.
- Remote Access dev URL: `http://127.0.0.1:5173/remote-access`.

## Search Hints

- App shell or navigation: start at `frontend/src/shared/layouts/AppShell.tsx` and `frontend/src/app/router.tsx`.
- Theme mode or tokens: start at `frontend/src/theme/AppThemeProvider.tsx`, `frontend/src/theme/ThemeModeContext.ts`, `frontend/src/theme/createAppTheme.ts`, and `frontend/src/theme/tokens.ts`.
- Placeholder route text: start under `frontend/src/features/*`.
- HTTP API clients: start under `frontend/src/api/*`; Image Lab calls live in `frontend/src/api/imageApi.ts`; Video Lab calls live in `frontend/src/api/videoApi.ts`.
- Image Lab UI/runtime flow: start at `frontend/src/features/image-lab/ImageLabPage.tsx` and `frontend/src/runtime/fileAdapter.ts`.
- Pipeline Flow UI/API flow: start at `frontend/src/features/pipeline-flow/PipelineFlowPage.tsx`, `frontend/src/api/pipelineApi.ts`, `backend/src/vision/PipelineExecutor.*`, and `docs/PIPELINE_SCHEMA.md`.
- Video Lab UI/runtime flow: start at `frontend/src/features/video-lab/VideoLabPage.tsx`, `frontend/src/api/videoApi.ts`, and `frontend/src/runtime/fileAdapter.ts`; optical-flow/stabilization preview/export, motion metrics, and template-match tracking live in `backend/src/video/VideoService.*`.
- Remote Access UI/API: start at `frontend/src/features/remote-access/RemoteAccessPage.tsx`, `frontend/src/api/remoteApi.ts`, and `backend/src/security/RemoteAccessManager.*`.
- Dashboard/showcase tables and charts: start at `frontend/src/features/dashboard/DashboardPage.tsx`, `frontend/src/features/chart-showcase/ChartShowcasePage.tsx`, `frontend/src/features/data-grid/DataGridPage.tsx`, `frontend/src/api/jobsApi.ts`, `frontend/src/api/logsApi.ts`, and `frontend/src/api/filesApi.ts`.
- Backend health/static server: start at `backend/src/server/ApiServer.cpp` and `backend/src/main.cpp`.
- Backend WebSocket events: start at `backend/src/server/EventHub.*` and `backend/src/server/WebSocketGateway.*`.
- Backend jobs/logs: start at `backend/src/jobs/JobQueue.*` and `backend/src/logging/LogStore.*`.
- Backend Image Lab processing: start at `backend/src/image/ImageResultStore.*` and `backend/src/image/ImageFilters.*`.
- Backend Video Lab processing: start at `backend/src/video/VideoService.*` and `backend/src/server/ApiServer.cpp` video routes.
- Desktop app mode: start at `backend/src/host/WebViewHost.cpp`.
- Build/debug/publish workflow: start at `scripts/bootstrap.ps1`, `scripts/build.ps1`, `scripts/prepare-debug.ps1`, `scripts/publish.ps1`, and `docs/BUILD_AND_DEBUG_POLICY.md`.
