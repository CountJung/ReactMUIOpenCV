# 빌드 및 디버깅 정책

이 문서는 React + TypeScript + MUI 프론트엔드와 C++ + CMake + OpenCV 백엔드를 함께 사용하는 이 프로젝트의 빌드/디버깅 기준입니다. 비슷한 구조의 프로젝트를 새로 만들거나 지침과 스킬을 마이그레이션할 때도 같은 원칙을 유지합니다.

## 기본 원칙

- Release 빌드 구현은 `scripts\build.ps1` 아래에 둡니다. 루트에서 `.\build.ps1`을 실행해도 같은 Release 모드 앱이 빌드되어야 하며, 이 루트 파일은 호환용 위임 스크립트로 유지합니다.
- VS Code 디버깅은 실행 대상 파일이 없다는 오류 없이 바로 시작할 수 있어야 합니다.
- 디버깅 시작 전 프론트엔드 의존성, 타입 검사, 백엔드 CMake 구성, Debug 빌드가 자동으로 준비되어야 합니다.
- 정적 UI를 서빙하는 WebView2 앱 모드와 백엔드 디버그는 시작 전 `frontend/dist`를 최신 소스 기준으로 빌드해야 합니다.
- 디버그 시작 전 같은 워크스페이스의 이전 런타임 프로세스가 남아 있으면 종료하여 오래된 서버에 붙지 않도록 해야 합니다.
- 앱 모드와 웹 모드를 모두 지원하는 프로젝트는 각각 명확한 디버그 구성을 제공합니다.
- WebView2 앱 모드와 브라우저 웹 모드는 같은 React UI를 로드해야 합니다.

## VS Code 디버깅

VS Code 디버그 구성은 새 체크아웃 상태에서도 준비 작업을 거치면 바로 실행 가능해야 합니다.

필수 동작:

- 데스크톱 앱 모드는 WebView2 또는 플랫폼 앱 호스트로 `http://127.0.0.1:18730`을 로드합니다.
- 웹 모드는 로컬 C++ 백엔드와 브라우저 UI를 함께 사용할 수 있어야 합니다.
- 프론트엔드 디버그 시작 전 `npm install` 필요 여부를 확인합니다.
- 프론트엔드 디버그 시작 전 TypeScript 타입 검사를 실행합니다.
- WebView2 앱 모드와 정적 백엔드 디버그 시작 전 `frontend/dist` 번들을 생성합니다.
- WebView2 앱 모드와 백엔드 디버그 시작 전 같은 워크스페이스의 기존 `ReactMUIOpenCV.exe` 또는 `ReactMUIOpenCVApp.exe`를 종료합니다.
- 백엔드 디버그 시작 전 vcpkg가 준비되어 있는지 확인합니다.
- 백엔드 디버그 시작 전 설치된 Visual Studio 중 CMake가 지원하는 최신 generator를 선택하고 CMake configure를 실행합니다.
- 기존 generated build tree의 CMake generator가 달라졌으면 `backend/out/build` 아래의 CMake cache만 정리하고 다시 configure합니다.
- 백엔드 디버그 시작 전 Debug 실행 파일을 빌드합니다.
- `launch.json`은 명시적인 실행 파일 경로를 사용합니다.
- prelaunch 스크립트는 실행 파일이 없으면 명확한 오류 메시지로 실패해야 합니다.

편집기 전역의 선택된 CMake 대상에 의존하지 않습니다. 빌드 시스템이 생성하는 안정적인 경로를 사용하고, prelaunch task가 그 경로를 검증합니다.

## 루트 Release 빌드

Release 빌드 명령:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1
```

필수 동작:

- 프론트엔드 의존성을 확인하거나 설치합니다.
- `frontend/dist` 프로덕션 번들을 생성합니다.
- 백엔드 CMake 구성을 수행합니다.
- Release 백엔드 실행 파일을 빌드합니다.
- WebView2 SDK가 있으면 데스크톱 앱 호스트도 Release로 빌드합니다.
- 예상 Release 실행 파일이 없으면 명확하게 실패합니다.

호환용 루트 명령도 계속 지원합니다.

```powershell
.\build.ps1
```

## 게시 빌드

외부 배포가 필요하면 `/publish` 아래에 게시물을 생성합니다.

필수 동작:

- 명시적으로 건너뛰지 않는 한 Release 빌드를 먼저 수행합니다.
- 자체 실행 가능한 번들 폴더를 만듭니다.
- 백엔드 실행 파일과 필요한 런타임 DLL을 포함합니다.
- 앱 모드를 지원하면 데스크톱 앱 호스트 실행 파일을 포함합니다.
- 빌드된 React UI를 포함합니다.
- 앱 모드, 웹 모드, LAN 웹 모드 시작 스크립트를 구분해서 제공합니다.
- 버전 zip과 `latest` zip을 생성합니다.
- `/docs`에는 사용자 가이드, 개발자 초기 셋업 가이드, 게시 가이드, 코딩 가이드가 포함되어야 합니다.

## 점검 체크리스트

- `scripts\build.ps1`과 호환용 `.\build.ps1`이 루트에서 성공하는지 확인합니다.
- `scripts\prepare-debug.ps1 -BuildFrontend -StopExistingRuntime`가 이전 워크스페이스 런타임을 정리하고 `frontend/dist`와 Debug 백엔드 실행 파일을 생성하는지 확인합니다.
- `scripts\prepare-debug.ps1 -SkipFrontend`가 프론트엔드 번들을 건너뛰고 Debug 백엔드 실행 파일만 생성하는지 확인합니다.
- `scripts\prepare-debug.ps1 -SkipBackend`가 프론트엔드 의존성과 타입 검사를 통과하는지 확인합니다.
- VS Code 앱 모드 디버그가 `ReactMUIOpenCVApp.exe`를 찾는지 확인합니다.
- VS Code 백엔드 디버그가 `ReactMUIOpenCV.exe`를 찾는지 확인합니다.
- `http://127.0.0.1:18730/api/health`가 `ok`를 반환하는지 확인합니다.
