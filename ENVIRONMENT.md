# Development Environment

This project is prepared for a Windows-first React + WebView2 + C++ OpenCV workflow.

For a detailed Korean first-time setup and debugging walkthrough, see `docs/DEVELOPER_SETUP_GUIDE.md`.

## Recommended Toolchain

- Windows 10 or 11 x64
- Visual Studio 2022 Build Tools with MSVC and Windows SDK
- CMake 3.26 or newer
- Git
- Node.js 22 LTS with npm 10 or newer
- vcpkg for C++ dependencies
- Microsoft Edge WebView2 Runtime

## Frontend Setup

```powershell
cd frontend
npm install
npm run dev
```

The dev server uses `http://127.0.0.1:5173`. Production frontend files are emitted to `frontend/dist`.

## One-command Release Build

From the project root:

```powershell
.\build.ps1
```

The root build script conditionally installs frontend dependencies, builds `frontend/dist`, configures the vcpkg CMake preset, and builds the Release backend executable at:

```txt
backend/out/build/windows-msvc-vcpkg/Release/ReactMUIOpenCV.exe
```

## Publish Bundle

Create a portable install bundle under `/publish`:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\publish.ps1
```

The publish output includes:

```txt
publish/ReactMUIOpenCV/
publish/ReactMUIOpenCV-{version}.zip
publish/ReactMUIOpenCV-latest.zip
```

Upload the zip files to an external web server for distribution. See `docs/PUBLISHING.md` and `docs/USER_GUIDE.md`.

## Backend Setup

Set `VCPKG_ROOT` to your vcpkg checkout:

```powershell
$env:VCPKG_ROOT="D:\dev\vcpkg"
```

Install manifest dependencies and build with CMake:

```powershell
cd backend
vcpkg install
cmake --preset windows-msvc-vcpkg
cmake --build --preset windows-msvc-vcpkg-debug
```

Run the backend:

```powershell
.\out\build\windows-msvc-vcpkg\Debug\ReactMUIOpenCV.exe
```

The backend listens on `http://127.0.0.1:18730` by default. Passing `--lan` binds to `0.0.0.0`; keep that mode behind explicit user confirmation and authentication when the real LAN workflow is implemented.

## Runtime Modes

Desktop app mode:

```powershell
.\backend\out\build\windows-msvc-vcpkg\Debug\ReactMUIOpenCVApp.exe
```

The desktop app host starts the local backend when needed and loads `http://127.0.0.1:18730` in WebView2.

Web mode:

```powershell
.\backend\out\build\windows-msvc-vcpkg\Debug\ReactMUIOpenCV.exe
```

Then open:

```txt
http://127.0.0.1:18730
```

## VS Code Debugging

Use the compound launch configurations:

```txt
Debug Desktop App
Debug Frontend + Backend
Debug Remote Access + Backend LAN
```

VS Code runs preparation tasks before debugging:

- Frontend debug checks `node_modules`, installs when needed, runs `npm run typecheck`, then starts Vite on `http://127.0.0.1:5173`.
- Backend debug bootstraps workspace-local vcpkg when needed, configures CMake, builds the Debug backend, verifies `backend/out/build/windows-msvc-vcpkg/Debug/ReactMUIOpenCV.exe`, and then enters `cppvsdbg`.
- Desktop app debug builds and verifies `backend/out/build/windows-msvc-vcpkg/Debug/ReactMUIOpenCVApp.exe`, then enters `cppvsdbg` for the app host.
- Backend launch uses the explicit Debug executable path, so selecting a CMake launch target in VS Code is not required.

## Compatibility Notes

- Prefer the vcpkg manifest over ad hoc system library paths.
- Prefer MSVC x64 for the WebView2 desktop target.
- Keep frontend dependencies installed from `frontend/package.json`; do not commit `node_modules`.
- Keep generated CMake output under `backend/out`.
