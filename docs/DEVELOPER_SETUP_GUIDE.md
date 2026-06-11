# 개발자 초기 셋업 가이드

이 문서는 ReactMUIOpenCV를 새 Windows 개발 PC에서 처음부터 셋업하고, VS Code 디버깅과 Release 배포까지 이어 가는 절차를 설명합니다.

React + TypeScript 프론트엔드와 C++ + CMake + OpenCV 백엔드가 한 프로젝트 안에서 함께 동작하므로, 직접 명령을 흩어서 실행하기보다 이 저장소가 제공하는 VS Code task와 PowerShell 스크립트를 우선 사용합니다.

## 목표 구조

이 프로젝트는 두 실행 경험을 동시에 유지합니다.

```txt
개발 중 웹 모드
├─ Vite dev server: http://127.0.0.1:5173
├─ C++ HTTP API:    http://127.0.0.1:18730
└─ C++ WebSocket:   ws://127.0.0.1:18731

앱/배포 모드
├─ React production bundle: frontend/dist
├─ C++ backend static server: http://127.0.0.1:18730
└─ WebView2 desktop host: http://127.0.0.1:18730 로드
```

개발 중에는 Vite가 React UI를 빠르게 제공하고, API와 WebSocket은 C++ 백엔드가 제공합니다. 앱 모드와 배포 모드에서는 C++ 백엔드가 `frontend/dist`를 정적 파일로 제공합니다.

## 1. 필수 도구 설치

Windows 10/11 x64 기준으로 준비합니다.

- Visual Studio C++ Build Tools 또는 Visual Studio: MSVC x64, Windows SDK, C++ CMake tools 포함
- CMake 3.26 이상
- Git
- Node.js 20.19 이상 25 미만, npm 10 이상
- Visual Studio Code
- Microsoft Edge WebView2 Runtime
- WebView2 SDK: 앱 호스트 타깃을 빌드하려면 필요

VS Code 권장 확장은 저장소의 `.vscode/extensions.json`에 있습니다. VS Code에서 이 저장소를 열면 추천 확장 설치 알림을 수락합니다.

현재 `backend/CMakePresets.json`의 기본 generator는 `Visual Studio 18 2026`입니다. 설치된 Visual Studio 버전과 맞지 않으면 CMake configure가 실패하므로, 사용하는 MSVC 버전에 맞게 preset generator를 조정하거나 해당 Visual Studio 버전을 설치합니다.

## 2. 저장소 열기

PowerShell에서 저장소 루트로 이동합니다.

```powershell
cd D:\ArielNetworks\Predevelop\CPP\ReactMUIOpenCV
code .
```

앞으로의 명령은 특별히 경로가 표시되지 않는 한 저장소 루트에서 실행합니다.

## 3. 스크립트가 준비하는 자원

이 프로젝트는 새 체크아웃 후 필요한 자원을 다음 스크립트로 준비합니다.

| 자원 | 담당 스크립트 또는 task | 생성/확인 위치 |
| --- | --- | --- |
| React npm 패키지 | `scripts\ensure-frontend-deps.ps1` | `frontend\node_modules` |
| TypeScript 타입 검사 | `scripts\prepare-debug.ps1` | `npm run typecheck` |
| Debug 정적 UI 번들 | `scripts\prepare-debug.ps1 -BuildFrontend` | `frontend\dist` |
| 기존 워크스페이스 런타임 정리 | `scripts\prepare-debug.ps1 -StopExistingRuntime` | `backend\out\build` 아래 실행 중인 앱/백엔드 |
| workspace-local vcpkg | `scripts\setup-vcpkg.ps1` | `.tools\vcpkg\vcpkg.exe` |
| C++ manifest dependencies | CMake/vcpkg preset | `backend\vcpkg_installed` 및 build tree |
| Debug backend exe | `scripts\prepare-debug.ps1 -BuildFrontend` 또는 `-SkipFrontend` | `backend\out\build\windows-msvc-vcpkg\Debug\ReactMUIOpenCV.exe` |
| Debug app host exe | CMake WebView2 target | `backend\out\build\windows-msvc-vcpkg\Debug\ReactMUIOpenCVApp.exe` |
| React production bundle | `npm run build` 또는 `.\build.ps1` | `frontend\dist` |
| Release backend/app exe | `.\build.ps1` | `backend\out\build\windows-msvc-vcpkg\Release` |
| Publish bundle | `scripts\publish.ps1` | `publish\ReactMUIOpenCV` 및 zip |

처음 실행할 때는 npm 패키지 설치, vcpkg clone, C++ dependency build 때문에 시간이 걸릴 수 있습니다. 네트워크가 막혀 있으면 npm/vcpkg 단계에서 실패합니다.

## 4. 첫 Debug 준비

가장 먼저 전체 debug 준비 스크립트를 실행합니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\prepare-debug.ps1
```

이 명령은 다음을 순서대로 수행합니다.

1. `frontend/package.json`을 확인합니다.
2. `frontend/node_modules`가 없거나 오래되었으면 `npm install`을 실행합니다.
3. 기본 준비에서는 `npm run typecheck`로 프론트엔드 타입 검사를 실행합니다.
4. `.tools/vcpkg`가 없으면 vcpkg를 clone하고 bootstrap합니다.
5. `backend`에서 `cmake --preset windows-msvc-vcpkg`를 실행합니다.
6. `cmake --build --preset windows-msvc-vcpkg-debug`로 Debug 실행 파일을 빌드합니다.
7. `ReactMUIOpenCV.exe` 위치를 검증합니다.
8. WebView2 SDK가 있으면 `ReactMUIOpenCVApp.exe`도 준비됐는지 출력합니다.

부분 준비가 필요할 때는 다음 스위치를 사용합니다.

```powershell
# 프론트엔드만 준비: npm deps + typecheck
powershell -ExecutionPolicy Bypass -File .\scripts\prepare-debug.ps1 -SkipBackend

# 백엔드만 준비: vcpkg + CMake configure/build + exe 검증
powershell -ExecutionPolicy Bypass -File .\scripts\prepare-debug.ps1 -SkipFrontend

# 정적 UI까지 포함한 앱/백엔드 디버그 준비: 이전 런타임 정리 + frontend/dist + Debug exe 검증
powershell -ExecutionPolicy Bypass -File .\scripts\prepare-debug.ps1 -BuildFrontend -StopExistingRuntime

# 빠른 확인용: 프론트엔드 타입 검사는 건너뜀
powershell -ExecutionPolicy Bypass -File .\scripts\prepare-debug.ps1 -SkipTypecheck
```

## 5. VS Code에서 디버깅하기

가능하면 VS Code Run and Debug 패널의 구성을 사용합니다. 각 구성은 필요한 prelaunch task를 자동으로 실행합니다.

### Debug Web Mode

구성 이름:

```txt
Debug Web Mode (Frontend + Backend)
```

사용할 때:

- React 화면을 수정하면서 빠른 HMR이 필요할 때
- 브라우저 DevTools로 UI와 네트워크 요청을 같이 보고 싶을 때
- C++ 백엔드 breakpoint와 React UI 디버깅을 동시에 하고 싶을 때

동작:

- `Debug Backend`가 `debug: prepare backend` task를 실행합니다.
- C++ 백엔드를 `http://127.0.0.1:18730`에서 실행합니다.
- `Debug Frontend in Edge`가 `frontend: dev` task를 실행합니다.
- Vite UI를 `http://127.0.0.1:5173`에서 엽니다.
- 프론트엔드 API 기본값은 `http://127.0.0.1:18730`입니다.
- WebSocket 기본값은 현재 브라우저 hostname의 `18731` 포트입니다.

### Debug Desktop App

구성 이름:

```txt
Debug Desktop App
```

사용할 때:

- WebView2 앱 호스트 동작을 확인할 때
- 앱 실행 파일이 백엔드를 시작하고 `http://127.0.0.1:18730`을 로드하는지 확인할 때
- 브라우저가 아니라 실제 데스크톱 앱 모드에서 문제를 재현해야 할 때

주의:

- 이 구성은 `ReactMUIOpenCVApp.exe`를 실행합니다.
- 앱 호스트 빌드에는 WebView2 SDK가 필요합니다.
- prelaunch task가 같은 워크스페이스의 이전 런타임 프로세스를 종료하고, `frontend/dist`를 최신 소스 기준으로 빌드한 뒤 C++ Debug 실행 파일을 빌드합니다.
- `frontend/dist`가 없으면 `/api/health`는 동작하지만 `/`에는 UI 대신 "Build frontend/dist" 안내가 표시됩니다.

### Debug Backend

구성 이름:

```txt
Debug Backend
```

사용할 때:

- REST API, WebSocket, OpenCV 처리, JobQueue, logging만 디버깅할 때
- UI 없이 `http://127.0.0.1:18730/api/health`부터 확인할 때

백엔드가 뜨면 확인합니다.

```powershell
Invoke-RestMethod http://127.0.0.1:18730/api/health
```

### Debug LAN Web Mode

구성 이름:

```txt
Debug LAN Web Mode
```

사용할 때:

- `--lan` 바인딩, Remote Access 화면, QR/PIN/token 흐름을 확인할 때
- 같은 LAN의 다른 장치 접속을 테스트할 때

주의:

- 백엔드가 `0.0.0.0`에 바인딩되므로 신뢰할 수 있는 네트워크에서만 사용합니다.
- 외부 클라이언트는 인증과 read-only 기본 권한을 유지해야 합니다.
- Windows 방화벽이 포트를 막을 수 있습니다.

Vite LAN 개발 서버를 별도로 열어야 할 때는 VS Code task `frontend: dev lan`을 사용합니다. 모바일 브라우저가 Vite dev server에 직접 접속하는 경우 `127.0.0.1`은 모바일 자기 자신을 가리키므로, 필요하면 프론트엔드 환경 변수를 서버 PC 주소로 설정해서 실행합니다.

```powershell
cd frontend
$env:VITE_API_BASE_URL="http://<서버PC-IP>:18730"
$env:VITE_WS_BASE_URL="ws://<서버PC-IP>:18731/ws"
npm run dev:lan
```

## 6. VS Code Task 사용 흐름

Command Palette에서 `Tasks: Run Task`를 열어 다음 task를 실행할 수 있습니다.

| Task | 용도 |
| --- | --- |
| `project: install dependencies` | 프론트엔드 deps와 vcpkg 준비 |
| `debug: prepare frontend` | npm deps 확인 + TypeScript typecheck |
| `debug: prepare backend` | 이전 워크스페이스 런타임 정리 + `frontend/dist` 빌드 + vcpkg 준비 + CMake configure + Debug build |
| `frontend: dev` | Vite dev server 실행 |
| `frontend: dev lan` | LAN용 Vite dev server 실행 |
| `frontend: typecheck` | TypeScript 검사 |
| `frontend: lint` | ESLint 검사 |
| `backend: configure` | CMake configure |
| `backend: build` | Debug backend build |
| `backend: build release` | Release backend build |
| `project: build` | frontend build + backend Debug build |
| `project: build release` | 루트 `build.ps1` 실행 |
| `project: publish` | `scripts\publish.ps1` 실행 |

디버그 실행 파일 경로는 `.vscode/launch.json`에 명시되어 있습니다. 편집기 전역에서 선택한 CMake target에 의존하지 않습니다.

## 7. 수동 명령으로 확인하기

문제 원인을 좁힐 때는 아래 명령을 직접 실행합니다.

프론트엔드:

```powershell
cd frontend
npm run typecheck
npm run lint
npm run build
cd ..
```

백엔드:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup-vcpkg.ps1
cd backend
cmake --preset windows-msvc-vcpkg
cmake --build --preset windows-msvc-vcpkg-debug
cd ..
```

백엔드 실행:

```powershell
.\backend\out\build\windows-msvc-vcpkg\Debug\ReactMUIOpenCV.exe
```

또는 가장 최근 빌드된 backend exe를 실행합니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run-backend.ps1
```

LAN 모드:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run-backend.ps1 -Lan
```

## 8. Release 빌드

Release 빌드는 루트 스크립트를 사용합니다.

```powershell
.\build.ps1
```

이 스크립트는 다음을 수행합니다.

1. 프론트엔드 의존성을 확인하거나 설치합니다.
2. `npm run build`로 `frontend/dist`를 생성합니다.
3. `.tools/vcpkg`를 준비합니다.
4. CMake를 `windows-msvc-vcpkg` preset으로 configure합니다.
5. `windows-msvc-vcpkg-release` preset으로 Release backend를 빌드합니다.
6. WebView2 SDK가 있으면 Release app host도 빌드합니다.
7. `ReactMUIOpenCV.exe`가 없으면 실패합니다.
8. `ReactMUIOpenCVApp.exe`가 없으면 경고를 출력합니다.

선택적으로 일부 단계를 건너뛸 수 있습니다.

```powershell
.\build.ps1 -SkipFrontendInstall
.\build.ps1 -SkipFrontendBuild
.\build.ps1 -SkipBackendBuild
```

일반적으로 배포 전에는 건너뛰지 않는 전체 `.\build.ps1`을 사용합니다.

## 9. Publish 번들 생성

외부 배포용 portable bundle은 publish 스크립트로 생성합니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\publish.ps1
```

버전을 지정하려면:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\publish.ps1 -Version 0.1.0
```

이미 최신 Release 빌드가 준비된 경우:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\publish.ps1 -SkipBuild
```

publish 스크립트는 기본적으로 `.\build.ps1`을 먼저 실행하고 다음 결과를 만듭니다.

```txt
publish/
├─ ReactMUIOpenCV/
│  ├─ ReactMUIOpenCV.exe
│  ├─ ReactMUIOpenCVApp.exe
│  ├─ *.dll
│  ├─ frontend/dist/
│  ├─ docs/
│  ├─ README.md
│  ├─ Start-ReactMUIOpenCV.ps1
│  ├─ Start-ReactMUIOpenCV-Web.ps1
│  ├─ Start-ReactMUIOpenCV-LAN.ps1
│  └─ Install-ReactMUIOpenCV.ps1
├─ ReactMUIOpenCV-{version}.zip
└─ ReactMUIOpenCV-latest.zip
```

`publish.ps1`은 `ReactMUIOpenCVApp.exe`가 없으면 실패합니다. 배포 번들에는 데스크톱 앱 모드가 포함되어야 하므로, WebView2 SDK가 준비되어 있는지 먼저 확인합니다.

## 10. WebView2 SDK와 Runtime

두 가지를 구분합니다.

- WebView2 Runtime: 사용자의 Windows에서 WebView2 앱을 실행하기 위한 런타임입니다.
- WebView2 SDK: 개발자가 `ReactMUIOpenCVApp.exe`를 빌드하기 위한 헤더와 라이브러리입니다.

현재 CMake는 기본적으로 다음 SDK 경로를 찾습니다.

```txt
%USERPROFILE%\.nuget\packages\microsoft.web.webview2\1.0.3800.47\build\native
```

이 경로에 `include\WebView2.h`와 `x64\WebView2LoaderStatic.lib`가 있으면 `ReactMUIOpenCVApp` target이 생성됩니다. 없으면 백엔드 exe는 빌드되지만 앱 호스트 target은 건너뛰며 CMake warning이 출력됩니다.

SDK를 다른 위치에 둘 경우 CMake cache variable `WEBVIEW2_SDK_ROOT`를 해당 native SDK 경로로 지정합니다.

## 11. 정적 UI 서빙 방식

C++ 백엔드는 다음 순서로 React 정적 파일 위치를 찾습니다.

1. 실행 인자 `--static-root`
2. 실행 파일 옆 `frontend\dist`
3. 실행 파일 옆 `dist`
4. 현재 작업 디렉터리의 `frontend\dist`
5. CMake compile definition의 `APP_STATIC_ROOT`

개발 중 backend exe만 직접 실행했는데 React UI가 보이지 않으면 보통 `frontend/dist`가 없기 때문입니다. 다음을 실행합니다.

```powershell
cd frontend
npm run build
cd ..
```

또는 backend 실행 시 명시적으로 지정합니다.

```powershell
.\backend\out\build\windows-msvc-vcpkg\Debug\ReactMUIOpenCV.exe --static-root .\frontend\dist
```

## 12. 첫 셋업 체크리스트

- VS Code 추천 확장을 설치했습니다.
- Node/npm 버전이 `frontend/package.json`의 engines 범위와 맞습니다.
- CMake generator가 설치된 Visual Studio 버전과 맞습니다.
- `powershell -ExecutionPolicy Bypass -File .\scripts\prepare-debug.ps1`가 성공했습니다.
- `backend\out\build\windows-msvc-vcpkg\Debug\ReactMUIOpenCV.exe`가 존재합니다.
- 앱 모드가 필요하면 `ReactMUIOpenCVApp.exe`도 존재합니다.
- `Invoke-RestMethod http://127.0.0.1:18730/api/health`가 `status: ok`를 반환합니다.
- React 개발 모드는 `http://127.0.0.1:5173`에서 열립니다.
- 앱/정적 모드가 필요하면 `frontend/dist`가 생성되어 있습니다.
- Release 배포 전 `.\build.ps1`과 `scripts\publish.ps1`이 성공합니다.

## 13. 흔한 문제

### CMake configure가 generator 오류로 실패

`backend/CMakePresets.json`의 generator와 설치된 Visual Studio 버전이 맞는지 확인합니다. MSVC x64와 Windows SDK가 설치되어 있어야 합니다.

### vcpkg clone 또는 npm install 실패

처음 셋업은 네트워크가 필요합니다. 사내 프록시나 방화벽이 있으면 GitHub, npm registry, vcpkg dependency 다운로드가 허용되어야 합니다.

### 앱 모드 exe가 생성되지 않음

WebView2 SDK가 없으면 `ReactMUIOpenCVApp` target이 생성되지 않습니다. CMake warning에서 WebView2 SDK 경로를 확인하고 `WEBVIEW2_SDK_ROOT`를 맞춥니다.

### WebView2 앱은 열리지만 UI가 보이지 않음

백엔드가 `frontend/dist`를 찾지 못했을 수 있습니다. `npm run build` 또는 `.\build.ps1`을 실행한 뒤 다시 시작합니다.

### WebView2 앱이 수정 전 화면을 계속 보여줌

이전 `ReactMUIOpenCV.exe`가 `18730` 포트에 남아 있으면 앱 호스트가 새로 빌드된 백엔드 대신 기존 서버에 붙을 수 있습니다. VS Code의 `Debug Desktop App`은 prelaunch task에서 같은 워크스페이스의 이전 런타임을 정리합니다. 수동으로 정리하려면 다음을 실행합니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\prepare-debug.ps1 -BuildFrontend -StopExistingRuntime
```

### Vite 화면은 뜨지만 API가 실패

`Debug Backend`가 실행 중인지 확인합니다. API 기본 주소는 `http://127.0.0.1:18730`입니다.

### 모바일 LAN 개발에서 API가 실패

모바일에서 `127.0.0.1`은 개발 PC가 아니라 모바일 장치 자신입니다. `VITE_API_BASE_URL`과 `VITE_WS_BASE_URL`을 개발 PC LAN IP로 지정해 Vite LAN 서버를 실행합니다.
