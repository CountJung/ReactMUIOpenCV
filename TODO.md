# Project TODO

This TODO is derived from `MasterPlan.md` and is ordered by dependency, not by screen name.

## Phase 0 - Scaffold The Runtime Spine

- [x] Create `frontend/` with React, Vite, TypeScript, MUI, React Router, TanStack Query, and the planned source folders.
- [x] Create `backend/` with CMake, C++20 or C++23, OpenCV discovery, and the planned source folders.
- [x] Implement the C++ app entrypoint that starts the local HTTP server on `127.0.0.1:18730`.
- [x] Serve the React build output from the C++ static file server.
- [x] Add `GET /api/health` and a typed frontend health API client.
- [x] Add a minimal WebView2 host that loads `http://127.0.0.1:18730`.
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
- [x] Refactor the backend from a monolithic `main.cpp` into class-based source files under `common`, `server`, `security`, `jobs`, `logging`, `storage`, and `image`.

## Phase 4 - Image Lab

- [x] Add Desktop local image open flow through the runtime file adapter.
- [x] Add browser upload flow for LAN clients where allowed.
- [x] Implement OpenCV image operations: open, save, resize, crop, rotate, grayscale, blur, sharpen, threshold, edge detect, contour detect, histogram, color conversion, and compare.
- [x] Store image results by `resultId` and expose preview retrieval.
- [x] Build before/after comparison UI with parameter controls and error states.
- [x] Ensure mobile can inspect results without arbitrary local file access.

## Phase 5 - Video Lab

- [x] Implement video open, metadata extraction, frame read, and preview frame APIs.
- [x] Add frame extraction and frame-by-frame filter jobs.
- [x] Stream progress over WebSocket.
- [x] Add export flow with codec failure logging.
- [x] Separate preview-frame processing from full-video export.

## Phase 6 - Vision Pipeline Flow

- [x] Define React Flow node types for input, processing, and output nodes.
- [x] Define pipeline JSON schema shared by frontend and backend.
- [x] Implement `PipelineExecutor` in C++.
- [x] Cache intermediate step results.
- [x] Broadcast node execution events and sync execution state across clients.
- [x] Limit complex node editing on mobile.

## Phase 7 - Dashboard And Showcase

- [x] Build dashboard cards for backend status, active jobs, remote clients, recent logs, and recent results.
- [x] Add chart showcase for processing statistics.
- [x] Add data grid for job history and file/result library views.
- [x] Keep all dashboard state server-owned where multiple clients must agree.

## Phase 8 - Packaging And Template

- [x] Automate frontend build and copy into backend static assets.
- [x] Automate backend build and package output.
- [x] Publish an external web-server-ready portable install bundle under `/publish`.
- [x] Check WebView2 Runtime availability.
- [x] Add user-facing firewall guidance for LAN Web UI Mode.
- [x] Document LAN usage and developer extension points.

## Phase 9 - LearnOpenCV Example Implementation Packs

These packs are selected from `spmallick/learnopencv` because they fit this project's C++ OpenCV image, video, pipeline, and dashboard goals. Each pack is sized so one focused agent can implement the backend service/API, frontend UI, docs/TODO update, and verification without expanding into unrelated examples.

### Pack 9A - Video Fundamentals And Diagnostics

- [x] Add a Video Lab sample for read/write/display video basics based on LearnOpenCV `Read, Write and Display a video using OpenCV`.
- [x] Add frame-rate measurement and display diagnostics based on LearnOpenCV `How to find frame rate or frames per second`.
- [x] Surface Video Lab diagnostics in Charts/Data Grid so measured FPS and metadata can be compared with job history.
- Agent focus: `$cpp-opencv-backend` for `VideoService` diagnostics and codec-safe export checks, `$react-mui-frontend` for Video Lab and chart/table display.

### Pack 9B - Motion And Stabilization Pipelines

- [x] Add Video Lab stabilization preview/export and pipeline node based on LearnOpenCV `Video Stabilization Using Point Feature Matching in OpenCV`.
- [x] Add optical-flow preview and pipeline node based on LearnOpenCV `Optical Flow in OpenCV`.
- [x] Add dashboard metrics for tracked features, flow magnitude, stabilization crop, and processing time.
- Agent focus: backend video/vision worker first, then React Flow node editor and execution result UI.

### Pack 9C - Tracking And Detection Nodes

- [x] Add object tracking pipeline nodes based on LearnOpenCV `Object Tracking using OpenCV` and `MultiTracker`.
- [x] Store per-frame tracking metadata for Data Grid inspection.
- [x] Add simple start-frame ROI selection UI before enabling long tracking jobs.
- Agent focus: backend tracking service, metadata schema, Video Lab/Pipeline UI, job cancellation behavior.

### Pack 9D - Alignment, Calibration, And QR Utilities

- [ ] Add feature-based alignment/image registration operation based on LearnOpenCV `Image Alignment (Feature Based)` and `Image Alignment (ECC)`.
- [ ] Add camera calibration utility and calibration-result storage based on LearnOpenCV `Camera Calibration using OpenCV`.
- [ ] Add QR code scanner utility based on LearnOpenCV `OpenCV QR Code Scanner`.
- Agent focus: image/vision services, persistent calibration artifacts under backend-owned `data/`, Image Lab utility panels.

### Pack 9E - Shape Analysis And Classical Vision

- [ ] Add shape-analysis operations for blob centroid, convex hull, Hu moments, and Hough transform based on LearnOpenCV examples.
- [ ] Show result overlays and structured shape metadata in Data Grid.
- [ ] Add corresponding pipeline operation nodes for reusable inspections.
- Agent focus: image filters/vision metadata, preview overlays, table schema.

### Pack 9F - Advanced Image Composition And Rendering

- [ ] Add inpainting, seamless cloning, alpha blending, exposure fusion, HDR, and non-photorealistic rendering operations as Image Lab advanced filters.
- [ ] Add operation-specific parameter controls and before/after examples.
- Agent focus: Image Lab filters with small verified parameter sets, output save/export behavior.

### Pack 9G - Optional DNN Examples

- [ ] Add DNN-based examples behind optional model assets: face detection, YOLO object detection, text detection, pose estimation, and Mask R-CNN.
- [ ] Add model asset discovery, missing-model guidance, and guarded execution so default builds remain lightweight.
- Agent focus: optional model management, backend validation, security/path checks, UI guidance.

### Pack 9H - Performance Instrumentation

- [ ] Add performance demo for parallel pixel access with OpenCV `forEach` and compare it with existing filter implementations.
- [ ] Record benchmark samples as jobs and chart them in Dashboard/Charts.
- Agent focus: backend benchmark service, job/event integration, chart visualization.

## Guardrails

- [ ] Keep LAN Web UI Mode off by default.
- [ ] Never allow unauthenticated external control.
- [ ] Keep arbitrary local path access Desktop-only.
- [ ] Isolate uploaded files in a temp directory.
- [ ] Prevent path traversal on every file API.
- [ ] Run long OpenCV work through the job queue, not the UI or request thread.
- [ ] Avoid repeatedly sending large image or video payloads as base64.
