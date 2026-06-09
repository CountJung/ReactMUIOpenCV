# Publishing Guide

This guide explains how to create a Windows bundle that can be uploaded to an external web server.

## Build And Publish

From the project root:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\publish.ps1
```

The publish script runs the root Release build first, then creates `/publish` artifacts:

```txt
publish/
├─ ReactMUIOpenCV/
│  ├─ ReactMUIOpenCV.exe
│  ├─ *.dll
│  ├─ frontend/dist/
│  ├─ docs/
│  ├─ README.md
│  ├─ Start-ReactMUIOpenCV.ps1
│  ├─ Start-ReactMUIOpenCV-LAN.ps1
│  └─ Install-ReactMUIOpenCV.ps1
├─ ReactMUIOpenCV-{version}.zip
└─ ReactMUIOpenCV-latest.zip
```

Use a custom version label:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\publish.ps1 -Version 0.1.0
```

Skip the build step when Release outputs are already fresh:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\publish.ps1 -SkipBuild
```

## Web Server Upload

Upload these files from `/publish`:

```txt
ReactMUIOpenCV-latest.zip
ReactMUIOpenCV-{version}.zip
```

Optionally upload the expanded `ReactMUIOpenCV/` folder if the server should expose individual files.

Recommended server headers:

```txt
Content-Type: application/zip
Cache-Control: no-cache for ReactMUIOpenCV-latest.zip
Cache-Control: public, max-age=31536000 for versioned zip files
```

## Verification Checklist

Before publishing externally:

- Run `scripts/publish.ps1`.
- Extract `publish/ReactMUIOpenCV-latest.zip` to a temporary folder.
- Run `Start-ReactMUIOpenCV.ps1`.
- Verify `http://127.0.0.1:18730/api/health` returns `ok`.
- Verify `http://127.0.0.1:18730` shows the React UI.
- Confirm the bundle includes `frontend/dist`.

## Current Packaging Scope

The current package is a portable zip with a per-user PowerShell installer. It does not yet create an MSI or MSIX package.

Future packaging options:

- MSI via WiX Toolset
- NSIS installer
- MSIX package with WebView2 Runtime checks
- Code signing for public distribution
