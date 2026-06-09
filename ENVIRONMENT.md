# Development Environment

This project is prepared for a Windows-first React + WebView2 + C++ OpenCV workflow.

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

## Compatibility Notes

- Prefer the vcpkg manifest over ad hoc system library paths.
- Prefer MSVC x64 for the WebView2 desktop target.
- Keep frontend dependencies installed from `frontend/package.json`; do not commit `node_modules`.
- Keep generated CMake output under `backend/out`.
