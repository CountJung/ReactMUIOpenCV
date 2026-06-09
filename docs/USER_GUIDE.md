# ReactMUIOpenCV User Guide

ReactMUIOpenCV is a Windows-first local vision/media testbed. It supports two runtime modes:

- Desktop app mode: opens the shared React UI inside a Windows WebView2 app window.
- Web mode: runs the local C++ OpenCV backend and opens the same UI in a browser at `http://127.0.0.1:18730`.

## Requirements

- Windows 10 or 11 x64
- Microsoft Edge WebView2 Runtime
- A trusted local network if LAN mode is used

The published bundle includes the desktop app host, backend executable, required runtime DLLs, and the built React UI. Node.js, CMake, and vcpkg are not required for end users.

## Run Desktop App Mode Without Installing

1. Download `ReactMUIOpenCV-latest.zip` from the published location.
2. Extract the zip to a local folder.
3. Right-click `Start-ReactMUIOpenCV.ps1` and run it with PowerShell.
4. The desktop app opens and loads the local UI through WebView2.

If PowerShell blocks the script, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\Start-ReactMUIOpenCV.ps1
```

## Run Web Mode Without Installing

Use web mode when you want to inspect the UI in a normal browser:

```powershell
powershell -ExecutionPolicy Bypass -File .\Start-ReactMUIOpenCV-Web.ps1
```

The browser opens `http://127.0.0.1:18730`.

## Install For Current User

From the extracted bundle:

```powershell
powershell -ExecutionPolicy Bypass -File .\Install-ReactMUIOpenCV.ps1
```

The installer copies the bundle to:

```txt
%LOCALAPPDATA%\Programs\ReactMUIOpenCV
```

It also creates a Start Menu shortcut named `ReactMUIOpenCV`.

## LAN Web UI Mode

LAN mode allows other devices on the same network to connect to the app host.

```powershell
powershell -ExecutionPolicy Bypass -File .\Start-ReactMUIOpenCV-LAN.ps1
```

Use LAN mode only on trusted networks. The backend binds to `0.0.0.0`, and Windows Firewall may ask for permission.

After startup, open:

```txt
http://127.0.0.1:18730/remote-access
```

Use the Remote Access screen for QR/token/PIN connection details.

## Troubleshooting

- If `http://127.0.0.1:18730` does not open, check whether another process is already using port `18730`.
- If desktop app mode does not open, install or repair Microsoft Edge WebView2 Runtime.
- If LAN clients cannot connect, check Windows Firewall and verify the devices are on the same network.
- If the UI shows only backend text, verify that `frontend/dist` exists inside the extracted bundle.
- Logs are written under the current working directory in `logs/backend.log`.
