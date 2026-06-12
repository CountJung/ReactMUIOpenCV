# 게시 가이드

이 문서는 ReactMUIOpenCV를 외부 웹 서버에 올릴 수 있는 Windows 휴대용 번들로 만드는 방법을 설명합니다.

## 빌드 및 게시

프로젝트 루트에서 실행합니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\publish.ps1
```

게시 스크립트는 루트 Release 빌드를 먼저 실행한 뒤 `/publish` 아래에 결과물을 만듭니다.
내부적으로는 `scripts\build.ps1`을 호출합니다.

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

버전 이름을 직접 지정하려면 다음처럼 실행합니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\publish.ps1 -Version 0.1.0
```

Release 산출물이 이미 최신이면 빌드 단계를 건너뛸 수 있습니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\publish.ps1 -SkipBuild
```

## 웹 서버 업로드

외부 웹 서버에는 보통 다음 파일을 올립니다.

```txt
ReactMUIOpenCV-latest.zip
ReactMUIOpenCV-{version}.zip
```

서버에서 개별 파일 다운로드도 제공해야 한다면 압축이 풀린 `ReactMUIOpenCV/` 폴더를 함께 업로드합니다.

권장 HTTP 헤더:

```txt
Content-Type: application/zip
Cache-Control: no-cache
```

버전이 고정된 zip 파일은 장기 캐시를 사용할 수 있습니다.

```txt
Cache-Control: public, max-age=31536000
```

## 게시 전 검증

외부에 올리기 전에 다음을 확인합니다.

- `scripts\publish.ps1`가 성공하는지 확인합니다.
- `publish\ReactMUIOpenCV-latest.zip`을 임시 폴더에 압축 해제합니다.
- `Start-ReactMUIOpenCV.ps1`을 실행해 WebView2 데스크톱 앱이 열리는지 확인합니다.
- `Start-ReactMUIOpenCV-Web.ps1`을 실행해 웹 모드가 열리는지 확인합니다.
- `http://127.0.0.1:18730/api/health`가 `ok`를 반환하는지 확인합니다.
- `http://127.0.0.1:18730`에서 React UI가 표시되는지 확인합니다.
- 번들 안에 `frontend/dist`가 포함되어 있는지 확인합니다.
- `docs/USER_GUIDE.md`, `docs/DEVELOPER_SETUP_GUIDE.md`, `docs/BUILD_AND_DEBUG_POLICY.md`, `docs/CODING_GUIDE.md`가 포함되어 있는지 확인합니다.

## 현재 패키징 범위

현재 패키지는 휴대용 zip과 현재 사용자용 PowerShell 설치 스크립트를 제공합니다. MSI 또는 MSIX 설치 패키지는 아직 만들지 않습니다.

향후 선택 가능한 배포 방식:

- WiX Toolset 기반 MSI
- NSIS 설치 프로그램
- WebView2 Runtime 확인을 포함한 MSIX
- 공개 배포용 코드 서명
