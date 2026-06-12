# Backend C++ Practices For ReactMUIOpenCV

Use this reference when editing `backend/src` C++ code. It captures project-specific defaults beyond general C++ style.

## Ownership Boundaries

- `main.cpp` stays a composition root: parse args, create services, wire dependencies, start runtimes, return status.
- `server/ApiServer.*` owns route registration only. Route handlers validate shape, check auth, call services, publish events, and translate responses.
- Durable behavior belongs in named services under `security`, `jobs`, `logging`, `storage`, `image`, `video`, or `vision`.
- CMake source lists must be updated in the same change when a new `.cpp` file is added.
- Prefer value members for light concrete services and RAII resources. Use heap ownership when objects are heavy, polymorphic, replaceable, or need stable lifetime across composition boundaries.
- Prefer `std::unique_ptr` for exclusive heap ownership. Use `std::shared_ptr` only for genuine shared ownership with a documented lifetime reason.
- In classes that own mutable state accessed by multiple threads, the owning class owns the lock. Keep the lock member beside the protected state in the header.
- For read-heavy stores and service registries, prefer `std::shared_mutex`: use `std::shared_lock` for reads and `std::unique_lock` for writes. Do not publish events or call callbacks while holding a state lock unless unavoidable and documented.
- Do not repeat general utility helpers inside individual `.cpp` files. Shared OpenCV conversion, ROI/rect JSON, string/file-name normalization, extension checks, and safe JSON extraction should live under `backend/src/common/*Utils.*`.

## Security Defaults

- Bind to `127.0.0.1` by default. Use `0.0.0.0` only for explicit LAN mode.
- Treat loopback requests as desktop/local control. Treat non-loopback requests as remote clients.
- Never expose PINs, access tokens, session tokens, or local filesystem paths to non-loopback clients unless a route is explicitly designed for that.
- Non-loopback mutating requests must require an authenticated control session. Read-only remote clients may use safe GET endpoints only.
- Local path open/save remains desktop-only. Browser uploads must be isolated to backend-owned temp/upload directories.
- Validate extensions before OpenCV decode/open. Normalize generated output filenames and never trust uploaded filenames as paths.
- Keep CORS permissive only while the API is local/LAN; pair exposed mutating routes with backend-side auth checks.

## Jobs And Events

- Long image, video, and pipeline work should run through `JobQueue` or a dedicated worker service, not directly on the HTTP request path.
- Publish `job.*`, `preview.*`, `log.appended`, `remote.client.*`, and `pipeline.node.*` events from the service boundary that owns the state change.
- A route that queues work should return `202` with the job record and enough metadata for the UI to invalidate/refetch.

## OpenCV Resource Rules

- Prefer RAII values: `cv::Mat`, `cv::VideoCapture`, `cv::VideoWriter`, standard containers, and smart pointers.
- Clone `cv::Mat` when storing or returning data across service boundaries.
- Use `common/OpenCvUtils.*` for repeated color conversion and ROI helpers before adding a new file-local OpenCV utility.
- Keep preview generation separate from full export. Preview should not lock a long-running export path.
- Log codec, decode, encode, metadata, and processing failures with enough context, but do not leak secrets or untrusted raw paths to remote clients.

## Windows And WebView2 Rules

- Wrap Win32 handles in a small RAII helper when a function opens more than one handle or has multiple exits.
- Prefer graceful backend shutdown when possible. If forced termination is used, keep it isolated to app-host-owned child processes.
- WebView2 user data lives under `%LOCALAPPDATA%/ReactMUIOpenCV/WebView2UserData`.

## Structure Thresholds

- When a source file exceeds 1000 lines, trigger `$project-structure-review-agent`.
- Before 1000 lines, split if a file mixes durable responsibilities. `ApiServer.cpp` may be route-dense, but processing/storage/security logic should move out as soon as it grows beyond thin orchestration.
