# ReactMUIOpenCV 사용자 가이드

ReactMUIOpenCV는 Windows 우선의 로컬 비전/미디어 테스트베드입니다. React UI, C++ 백엔드, OpenCV 처리 기능을 한 앱에서 사용합니다.

지원하는 실행 모드는 두 가지입니다.

- 데스크톱 앱 모드: Windows WebView2 창 안에서 같은 React UI를 엽니다.
- 웹 모드: 로컬 C++ OpenCV 백엔드를 실행하고 브라우저에서 `http://127.0.0.1:18730` UI를 엽니다.

## 요구 사항

- Windows 10 또는 Windows 11 x64
- Microsoft Edge WebView2 Runtime
- LAN 모드를 사용할 경우 신뢰할 수 있는 로컬 네트워크

게시 번들에는 데스크톱 앱 호스트, 백엔드 실행 파일, 필요한 런타임 DLL, 빌드된 React UI가 포함됩니다. 일반 사용자는 Node.js, CMake, vcpkg를 설치할 필요가 없습니다.

## 설치하지 않고 데스크톱 앱 모드 실행

1. 게시 위치에서 `ReactMUIOpenCV-latest.zip`을 내려받습니다.
2. zip 파일을 로컬 폴더에 압축 해제합니다.
3. `Start-ReactMUIOpenCV.ps1`을 PowerShell로 실행합니다.
4. 데스크톱 앱이 열리고 WebView2를 통해 로컬 UI가 표시됩니다.

PowerShell 실행 정책 때문에 차단되면 압축 해제 폴더에서 다음 명령을 실행합니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\Start-ReactMUIOpenCV.ps1
```

## 설치하지 않고 웹 모드 실행

일반 브라우저에서 UI를 확인하려면 웹 모드를 사용합니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\Start-ReactMUIOpenCV-Web.ps1
```

브라우저에서 다음 주소를 엽니다.

```txt
http://127.0.0.1:18730
```

## 현재 사용자에게 설치

압축 해제 폴더에서 실행합니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\Install-ReactMUIOpenCV.ps1
```

설치 스크립트는 번들을 다음 위치로 복사합니다.

```txt
%LOCALAPPDATA%\Programs\ReactMUIOpenCV
```

시작 메뉴에는 `ReactMUIOpenCV` 바로 가기가 만들어집니다.

## LAN 웹 UI 모드

LAN 모드는 같은 네트워크의 다른 장치가 앱에 접속할 수 있게 합니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\Start-ReactMUIOpenCV-LAN.ps1
```

LAN 모드는 신뢰할 수 있는 네트워크에서만 사용합니다. 이 모드에서는 백엔드가 `0.0.0.0`에 바인딩되며 Windows 방화벽 허용 여부를 물을 수 있습니다.

시작 후 로컬 장치에서 다음 화면을 엽니다.

```txt
http://127.0.0.1:18730/remote-access
```

Remote Access 화면에서 QR 코드, 토큰, PIN 연결 정보를 확인합니다.

## 주요 화면

- Dashboard: 백엔드 상태, 작업, 로그, 원격 접속 상태를 확인합니다.
- Remote Access: LAN 접속 상태와 인증 정보를 관리합니다.
- Image Lab: 이미지 열기, 업로드, 필터 적용, 저장을 수행합니다.
- Video Lab: 영상 열기, 메타데이터 확인, 프레임 미리보기, 프레임 추출, 필터 적용, 내보내기를 수행합니다.
- Pipeline Flow: 여러 비전 처리 단계를 파이프라인으로 구성할 예정입니다.
- Logs: 최근 백엔드 로그를 확인합니다.
- Settings: UI 테마와 설정을 관리합니다.

## 문제 해결

- `http://127.0.0.1:18730`이 열리지 않으면 다른 프로세스가 `18730` 포트를 사용 중인지 확인합니다.
- 데스크톱 앱 모드가 열리지 않으면 Microsoft Edge WebView2 Runtime을 설치하거나 복구합니다.
- LAN 클라이언트가 접속하지 못하면 Windows 방화벽과 같은 네트워크 연결 여부를 확인합니다.
- UI 대신 백엔드 텍스트만 보이면 압축 해제 폴더 안에 `frontend/dist`가 있는지 확인합니다.
- 로그는 실행 폴더의 `logs/backend.log`에 기록됩니다.
