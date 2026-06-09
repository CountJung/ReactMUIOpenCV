# 코딩 가이드

이 문서는 ReactMUIOpenCV를 사람이 직접 수정할 때 지켜야 할 개발 기준입니다. 목적은 여러 명이 작업해도 백엔드, 프론트엔드, 빌드/디버깅 흐름이 일관되게 유지되도록 하는 것입니다.

## 작업 전 확인

아키텍처나 기능을 변경하기 전에는 다음 문서를 먼저 읽습니다.

- `PROJECT_MAP.md`
- `MasterPlan.md`
- `TODO.md`
- `AGENTS.md`

파일을 찾을 때는 먼저 `PROJECT_MAP.md`의 Search Hints를 봅니다. 그래도 부족할 때만 `rg`로 검색합니다.

## 백엔드 구조

백엔드는 C++20/C++23, CMake, OpenCV 기반입니다.

원칙:

- `backend/src/main.cpp`는 composition root로만 둡니다.
- `main.cpp`에는 인자 파싱, 서비스 생성, 의존성 연결, 서버 시작 정도만 둡니다.
- 실제 기능은 책임별 클래스로 분리합니다.
- 각 클래스는 소유 영역 폴더 아래 `.h`와 `.cpp`로 나눕니다.
- 상속, 인터페이스, 소유 관계가 생기면 헤더에서 관계를 쉽게 확인할 수 있게 둡니다.
- HTTP route handler는 요청 검증, 서비스 호출, 이벤트 발행, 응답 변환만 담당합니다.

권장 폴더:

- `backend/src/common`: 공통 상수, 시간, 랜덤 ID, HTTP 유틸리티
- `backend/src/server`: REST, WebSocket, 정적 파일, 네트워크 정보
- `backend/src/security`: LAN 인증, 토큰, PIN, 세션
- `backend/src/jobs`: 장기 작업, 진행률, 취소
- `backend/src/logging`: 파일 로그, 최근 로그
- `backend/src/storage`: 설정, 파이프라인, 저장소성 상태
- `backend/src/image`: 이미지 OpenCV 처리
- `backend/src/video`: 비디오 OpenCV 처리
- `backend/src/vision`: 향후 비전 파이프라인 실행기
- `backend/src/host`: WebView2/데스크톱 호스트

## OpenCV 처리 규칙

- 이미지와 비디오의 원본/결과 상태는 백엔드가 소유합니다.
- 긴 작업은 UI나 요청 스레드에서 직접 처리하지 않고 job으로 분리합니다.
- 프리뷰 생성과 전체 export 처리는 별도 경로로 유지합니다.
- codec 실패, 파일 접근 실패, 파라미터 오류, 처리 실패는 로그에 남깁니다.
- 브라우저 업로드 파일은 격리된 업로드 폴더에 저장합니다.
- 임의 로컬 경로 접근은 데스크톱/loopback 요청에서만 허용합니다.
- 경로를 사용할 때는 정규화와 확장자 검증을 수행합니다.
- 큰 이미지나 비디오 payload를 base64로 반복 전송하지 않습니다.

## API와 이벤트

REST 응답은 기존 envelope 형식을 유지합니다.

```json
{
  "ok": true,
  "data": {},
  "error": null
}
```

오류 응답은 다음 형식을 사용합니다.

```json
{
  "ok": false,
  "data": null,
  "error": {
    "code": "error_code",
    "message": "사용자가 이해할 수 있는 메시지"
  }
}
```

WebSocket 이벤트는 서버 상태 변경, 작업 진행률, 로그, 프리뷰 갱신, 파이프라인 노드 상태를 프론트엔드에 전달합니다. 프론트엔드는 이벤트를 받아 TanStack Query 캐시를 갱신하거나 무효화합니다.

## 프론트엔드 구조

프론트엔드는 React, TypeScript, MUI, React Router, TanStack Query 기반입니다.

원칙:

- React 컴포넌트가 WebView2 API에 직접 의존하지 않게 합니다.
- 앱/웹/LAN 차이는 `frontend/src/runtime/*`에 둡니다.
- 백엔드 호출은 `frontend/src/api/*`의 typed API client로 감쌉니다.
- 라우트 화면은 `frontend/src/features/*` 아래에 둡니다.
- 공통 레이아웃과 재사용 컴포넌트는 `frontend/src/shared/*`에 둡니다.
- 색상, 여백, 그림자, 타이포그래피는 가능하면 MUI theme token을 사용합니다.
- 모바일에서는 상태 확인, 프리뷰, 로그, 단순 시작/중지 중심으로 설계합니다.

## UI 작성 기준

- 도구 버튼에는 가능한 경우 MUI 아이콘을 사용합니다.
- 숫자 값은 slider, stepper, number input 중 의미에 맞는 컨트롤을 사용합니다.
- 선택지는 select, tabs, segmented control처럼 익숙한 컨트롤을 사용합니다.
- 카드 안에 또 다른 카드를 중첩하지 않습니다.
- 화면의 주요 작업은 첫 화면에서 바로 수행할 수 있게 배치합니다.
- 텍스트가 모바일과 데스크톱에서 컨테이너를 넘치지 않는지 확인합니다.
- 운영 도구 성격의 화면은 과한 장식보다 스캔하기 쉬운 밀도와 명확한 상태 표시를 우선합니다.

## 빌드와 검증

프론트엔드 변경 후:

```powershell
cd frontend
npm run typecheck
npm run lint
npm run build
```

백엔드 변경 후:

```powershell
cd backend
cmake --build --preset windows-msvc-vcpkg-debug
```

루트 Release 빌드:

```powershell
.\build.ps1
```

게시 번들 생성:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\publish.ps1
```

VS Code 디버깅 전에는 prelaunch task가 의존성 확인과 Debug 빌드를 수행해야 합니다. 실행 파일이 없다는 오류가 나오면 `scripts\prepare-debug.ps1 -SkipFrontend` 또는 `debug: prepare backend` task를 먼저 확인합니다.

## 커밋 전 점검

- `PROJECT_MAP.md`가 새 파일, 폴더, 스크립트, 포트 변경을 반영하는지 확인합니다.
- `TODO.md`의 완료/예정 상태가 실제와 맞는지 확인합니다.
- `/docs` 문서가 사용자와 개발자에게 필요한 변경을 반영하는지 확인합니다.
- Debug 실행 파일과 Release 빌드 경로를 임의로 바꾸지 않았는지 확인합니다.
- LAN 접근, 로컬 경로 접근, 업로드 경로는 보안 규칙을 만족하는지 확인합니다.
