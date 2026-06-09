# Project TODO

This TODO is derived from `MasterPlan.md` and is ordered by dependency, not by screen name.

## Phase 0 - Scaffold The Runtime Spine

- [x] Create `frontend/` with React, Vite, TypeScript, MUI, React Router, TanStack Query, and the planned source folders.
- [x] Create `backend/` with CMake, C++20 or C++23, OpenCV discovery, and the planned source folders.
- [x] Implement the C++ app entrypoint that starts the local HTTP server on `127.0.0.1:18730`.
- [x] Serve the React build output from the C++ static file server.
- [x] Add `GET /api/health` and a typed frontend health API client.
- [ ] Add a minimal WebView2 host that loads `http://127.0.0.1:18730`.
- [ ] Verify the same UI loads in WebView2 and a desktop browser.

## Phase 1 - App Shell And Theme

- [x] Build the MUI app shell with sidebar, topbar, content area, and route layout.
- [x] Add `Dashboard`, `Remote Access`, `Image Lab`, `Video Lab`, `Pipeline Flow`, `Charts`, `Data Grid`, `Logs`, and `Settings` placeholder routes.
- [x] Implement theme tokens for palette, typography, shape, shadows, density, and component overrides.
- [x] Support `light`, `dark`, and `system` theme modes.
- [x] Persist theme settings locally and prepare backend settings sync.
- [x] Keep browser globals such as `window`, `document`, `localStorage`, and `matchMedia` behind safe client utilities.

## Phase 2 - LAN Web UI Mode

- [x] Add backend binding mode support for `127.0.0.1` and explicit `0.0.0.0`.
- [x] Detect LAN IP addresses and expose them through `GET /api/network-info`.
- [x] Implement Remote Access status APIs: status, enable, disable, rotate token, clients, disconnect all.
- [x] Add PIN or temporary token authentication with session timeout.
- [x] Render connection URL and QR code on the Remote Access screen.
- [x] Track connected remote clients and show their permission mode.
- [x] Default external clients to read-only mode.

## Phase 3 - API, Jobs, Logs, And Sync

- [x] Define shared API response and error conventions.
- [x] Implement REST route groups for system, settings, remote access, files, images, videos, pipelines, jobs, and logs.
- [x] Implement a WebSocket server and event envelope.
- [x] Broadcast `job.*`, `log.appended`, `backend.status.changed`, `remote.client.*`, `preview.*`, and `pipeline.node.*` events.
- [x] Add a backend job queue for long-running image and video work.
- [x] Add `spdlog` file logging and a recent-log API.
- [x] Wire frontend WebSocket events to TanStack Query cache invalidation or updates.

## Phase 4 - Image Lab

- [ ] Add Desktop local image open flow through the runtime file adapter.
- [ ] Add browser upload flow for LAN clients where allowed.
- [ ] Implement OpenCV image operations: open, save, resize, crop, rotate, grayscale, blur, sharpen, threshold, edge detect, contour detect, histogram, color conversion, and compare.
- [ ] Store image results by `resultId` and expose preview retrieval.
- [ ] Build before/after comparison UI with parameter controls and error states.
- [ ] Ensure mobile can inspect results without arbitrary local file access.

## Phase 5 - Video Lab

- [ ] Implement video open, metadata extraction, frame read, and preview frame APIs.
- [ ] Add frame extraction and frame-by-frame filter jobs.
- [ ] Stream progress over WebSocket.
- [ ] Add export flow with codec failure logging.
- [ ] Separate preview-frame processing from full-video export.

## Phase 6 - Vision Pipeline Flow

- [ ] Define React Flow node types for input, processing, and output nodes.
- [ ] Define pipeline JSON schema shared by frontend and backend.
- [ ] Implement `PipelineExecutor` in C++.
- [ ] Cache intermediate step results.
- [ ] Broadcast node execution events and sync execution state across clients.
- [ ] Limit complex node editing on mobile.

## Phase 7 - Dashboard And Showcase

- [ ] Build dashboard cards for backend status, active jobs, remote clients, recent logs, and recent results.
- [ ] Add chart showcase for processing statistics.
- [ ] Add data grid for job history and file/result library views.
- [ ] Keep all dashboard state server-owned where multiple clients must agree.

## Phase 8 - Packaging And Template

- [ ] Automate frontend build and copy into backend static assets.
- [ ] Automate backend build and package output.
- [ ] Check WebView2 Runtime availability.
- [ ] Add user-facing firewall guidance for LAN Web UI Mode.
- [ ] Document LAN usage and developer extension points.

## Guardrails

- [ ] Keep LAN Web UI Mode off by default.
- [ ] Never allow unauthenticated external control.
- [ ] Keep arbitrary local path access Desktop-only.
- [ ] Isolate uploaded files in a temp directory.
- [ ] Prevent path traversal on every file API.
- [ ] Run long OpenCV work through the job queue, not the UI or request thread.
- [ ] Avoid repeatedly sending large image or video payloads as base64.
