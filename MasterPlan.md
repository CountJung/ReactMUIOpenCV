# MASTER PLAN

# Windows App + Local Network Web App Server

## React + TypeScript + MUI + WebView2 + C++ OpenCV 범용 비전/미디어 테스트베드

---

## 1. 프로젝트 개요

이 프로젝트는 Windows 환경에서 실행되는 데스크톱 앱이면서 동시에 **로컬 네트워크 웹앱 서버**로 동작하는 범용 비전/미디어 테스트베드이다.

사용자는 노트북 또는 데스크톱 PC에서 앱을 실행하고, 해당 PC 안에서는 WebView2 기반 Windows 앱 UI를 통해 사용할 수 있다. 동시에 같은 Wi-Fi 또는 같은 LAN에 연결된 모바일, 태블릿, 다른 PC에서는 브라우저로 노트북의 로컬 주소에 접속하여 동일한 React UI를 확인하고 일부 기능을 조작할 수 있다.

핵심 구조는 다음과 같다.

```txt
Windows 실행 PC
├─ Windows Desktop App
│  └─ WebView2로 React UI 표시
│
├─ Local Web UI Server
│  └─ 같은 LAN의 모바일/태블릿/다른 PC 접속 허용
│
└─ C++ OpenCV Backend
   ├─ 이미지 처리
   ├─ 동영상 처리
   ├─ 컴퓨터비전 알고리즘
   ├─ Job Queue
   ├─ 로그
   ├─ 설정
   └─ WebSocket 실시간 상태 동기화
```

이 프로젝트의 목적은 단일 기능 앱을 만드는 것이 아니라, 향후 영상 편집기, 이미지 처리 도구, 비전 검사 장비 UI, 노드 기반 자동화 도구, 미디어 변환 도구, 제조/검사 대시보드 등으로 확장 가능한 **범용 기술 검증 플랫폼**을 만드는 것이다.

---

## 2. 최종 컨셉

## 2.1 핵심 컨셉

```txt
React UI = 공통 웹 UI
WebView2 = Windows 앱 내부에서 React UI를 띄우는 Host
C++ Backend = OpenCV 처리 + API 서버 + WebSocket 서버
LAN Web UI = 모바일/태블릿/다른 PC에서 접속하는 보조 클라이언트
```

즉, 이 프로젝트는 단순한 Windows 앱이 아니라 다음 두 가지 성격을 동시에 가진다.

```txt
1. Windows Desktop App
2. Local Network Web App Server
```

---

## 2.2 사용 시나리오

### 시나리오 1: 노트북에서 단독 사용

```txt
사용자
↓
Windows 앱 실행
↓
WebView2 안의 React UI 사용
↓
C++ OpenCV 백엔드에서 이미지/동영상 처리
```

### 시나리오 2: 모바일에서 상태 확인

```txt
노트북에서 앱 실행
↓
LAN Web UI Mode 활성화
↓
모바일에서 QR 코드 스캔
↓
모바일 브라우저로 접속
↓
처리 상태, 로그, 결과 이미지 확인
```

### 시나리오 3: 태블릿을 보조 모니터처럼 사용

```txt
Windows 앱에서 이미지 처리 작업 실행
↓
태블릿에서 Dashboard 또는 Preview 화면 접속
↓
처리 진행률과 결과를 실시간 확인
```

### 시나리오 4: 같은 네트워크의 다른 PC에서 제어

```txt
메인 노트북에서 C++ OpenCV 처리
↓
다른 PC 브라우저에서 접속
↓
이미지/동영상 작업 실행 또는 결과 확인
```

---

## 3. 핵심 목표

## 3.1 Desktop 목표

* Windows 앱으로 실행 가능해야 한다.
* WebView2 안에서 React UI를 표시한다.
* 로컬 C++ 백엔드와 안정적으로 통신한다.
* 이미지/동영상 파일을 로컬 경로 기반으로 처리할 수 있어야 한다.
* 파일 열기, 저장, 내보내기 등 데스크톱 앱다운 기능을 제공한다.

---

## 3.2 Local Network Web UI 목표

* 앱 실행 PC가 로컬 웹 서버 역할을 한다.
* 같은 Wi-Fi 또는 LAN 안의 모바일/태블릿/다른 PC에서 접속할 수 있어야 한다.
* 접속 URL과 QR 코드를 Windows 앱 화면에서 보여준다.
* 여러 클라이언트가 동일한 처리 상태를 볼 수 있어야 한다.
* 작업 진행률, 로그, 백엔드 상태를 WebSocket으로 실시간 동기화한다.
* 보안을 위해 LAN Web UI Mode는 기본값 OFF로 둔다.

---

## 3.3 OpenCV Backend 목표

* C++ 기반으로 이미지 처리, 동영상 처리, 컴퓨터비전 기법을 수행한다.
* OpenCV를 핵심 처리 엔진으로 사용한다.
* 무거운 작업은 Job Queue에서 비동기 처리한다.
* 처리 상태를 프론트엔드로 실시간 전달한다.
* 처리 결과, 로그, 에러를 UI에서 확인할 수 있게 한다.

---

## 3.4 UI Showcase 목표

* MUI 기반의 모던 UI를 구현한다.
* Light / Dark / System Theme을 지원한다.
* Hydration mismatch 방지 규칙을 명시한다.
* React Flow 기반 노드/플로우 편집기를 구현한다.
* 차트, 테이블, 로그 뷰어, 설정 화면 등 범용 UI 요소를 쇼케이스한다.
* 데스크톱, 태블릿, 모바일 화면에 맞는 반응형 레이아웃을 제공한다.

---

## 4. 최종 기술 스택

## 4.1 Frontend

```txt
React
TypeScript
Vite
MUI
MUI Theme System
React Router
TanStack Query
Zustand
React Flow
Apache ECharts 또는 Recharts
TanStack Table 또는 AG Grid
Monaco Editor
```

### 선택 이유

* React는 웹과 WebView2 양쪽에서 동일하게 사용할 수 있다.
* TypeScript는 대형 UI 프로젝트에서 타입 안정성을 제공한다.
* Vite는 빠른 개발 서버와 정적 빌드에 유리하다.
* MUI는 디자인 시스템, 테마, 컴포넌트 기반 UI 구현에 적합하다.
* React Flow는 플로우차트, 노드 에디터, 처리 파이프라인 UI 구현에 적합하다.
* TanStack Query는 API 상태 관리에 적합하다.
* Zustand는 가벼운 UI 상태 관리에 적합하다.

---

## 4.2 Desktop Host

```txt
Windows
Win32 또는 MFC Host
Microsoft Edge WebView2
```

### 역할

* Windows 실행 파일 제공
* WebView2 초기화
* React UI 표시
* 로컬 서버 시작/종료 관리
* LAN Web UI Mode 제어
* 시스템 트레이, 창 제어, 파일 선택 등 네이티브 기능 제공

---

## 4.3 Backend

```txt
C++20 또는 C++23
CMake
OpenCV
nlohmann/json
spdlog
SQLite
cpp-httplib 또는 Boost.Beast
WebSocket
```

### 역할

* 정적 React UI 파일 서빙
* REST API 제공
* WebSocket 실시간 이벤트 제공
* OpenCV 이미지/동영상 처리
* Job Queue 관리
* 로그 파일 저장
* 설정 저장
* 작업 이력 저장

---

## 5. 전체 아키텍처

```txt
┌─────────────────────────────────────────────────────────────┐
│ Windows 실행 PC                                             │
│                                                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ Windows Desktop Host                                  │  │
│  │                                                       │  │
│  │  ┌─────────────────────────────────────────────────┐  │  │
│  │  │ WebView2                                        │  │  │
│  │  │ http://127.0.0.1:18730                           │  │  │
│  │  │                                                 │  │  │
│  │  │ React + TypeScript + MUI UI                     │  │  │
│  │  └─────────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ Local Web UI Server                                   │  │
│  │                                                       │  │
│  │ Desktop 접속: http://127.0.0.1:18730                  │  │
│  │ LAN 접속:     http://{노트북IP}:18730                 │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ C++ OpenCV Backend                                    │  │
│  │                                                       │  │
│  │ REST API:   http://0.0.0.0:18731/api                  │  │
│  │ WebSocket: ws://0.0.0.0:18732/ws                      │  │
│  │                                                       │  │
│  │ - Image Processing                                    │  │
│  │ - Video Processing                                    │  │
│  │ - Vision Pipeline                                     │  │
│  │ - Job Queue                                           │  │
│  │ - Logging                                             │  │
│  │ - Settings                                            │  │
│  │ - SQLite Storage                                      │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              ▲
                              │
                              │ LAN / Wi-Fi
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌──────────────┐      ┌──────────────┐      ┌──────────────┐
│ Mobile       │      │ Tablet       │      │ Other PC     │
│ Browser      │      │ Browser      │      │ Browser      │
│              │      │              │      │              │
│ 접속 주소:   │      │ 접속 주소:   │      │ 접속 주소:   │
│ 노트북IP:포트 │      │ 노트북IP:포트 │      │ 노트북IP:포트 │
└──────────────┘      └──────────────┘      └──────────────┘
```

---

## 6. 실행 모드

## 6.1 Desktop Only Mode

기본 실행 모드이다.

```txt
UI Server Bind: 127.0.0.1
API Server Bind: 127.0.0.1
WebSocket Bind: 127.0.0.1
외부 기기 접속: 불가
```

특징:

* 가장 안전하다.
* 사용자 PC 내부에서만 접속 가능하다.
* 일반적인 Windows 앱처럼 동작한다.
* 기본값으로 사용한다.

---

## 6.2 LAN Web UI Mode

사용자가 명시적으로 켜는 모드이다.

```txt
UI Server Bind: 0.0.0.0
API Server Bind: 0.0.0.0
WebSocket Bind: 0.0.0.0
외부 기기 접속: 가능
```

특징:

* 같은 Wi-Fi 또는 LAN의 다른 기기에서 접속 가능하다.
* 접속 가능한 URL을 UI에 표시한다.
* QR Code를 제공한다.
* PIN 또는 Token 인증을 요구한다.
* 연결된 클라이언트를 확인할 수 있다.
* 필요 시 모든 클라이언트를 강제 종료할 수 있다.

---

## 7. 접속 URL 정책

## 7.1 Desktop 내부 접속

```txt
http://127.0.0.1:18730
```

WebView2는 위 주소로 React UI에 접속한다.

---

## 7.2 LAN 외부 접속

```txt
http://{노트북IP}:18730
```

예시:

```txt
http://192.168.0.25:18730
```

---

## 7.3 QR Code 접속

```txt
http://192.168.0.25:18730?token={accessToken}
```

QR Code에는 다음 정보를 포함할 수 있다.

```txt
- 접속 주소
- 임시 접근 토큰
- 세션 만료 시간
```

---

## 8. 보안 정책

## 8.1 기본 정책

```txt
LAN Web UI Mode는 기본값 OFF
사용자가 명시적으로 켜야 함
외부 접속 허용 시 경고 메시지 표시
PIN 또는 Token 인증 필수
세션 만료 시간 적용
접속 중인 클라이언트 목록 표시
Disconnect All 기능 제공
```

---

## 8.2 접근 제어

지원할 접근 제어 옵션:

```txt
- Access PIN
- Temporary Token
- Session Timeout
- Allowlist IP
- Read-only Mode
- Control Mode
```

---

## 8.3 권한 모드

### Read-only Mode

모바일/외부 클라이언트는 다음 기능만 사용할 수 있다.

```txt
- Dashboard 확인
- 처리 상태 확인
- 로그 확인
- 결과 미리보기
- 작업 이력 조회
```

### Control Mode

외부 클라이언트가 일부 제어 기능을 사용할 수 있다.

```txt
- 작업 시작
- 작업 중지
- 파이프라인 실행
- 간단한 설정 변경
```

### Restricted Mode

위험한 기능은 제한한다.

```txt
- 로컬 파일 삭제 금지
- 시스템 경로 접근 금지
- 임의 경로 저장 금지
- 백엔드 종료 금지
- 설정 초기화 금지
```

---

## 9. 파일 처리 정책

## 9.1 Desktop 파일 처리

Desktop에서는 로컬 경로 기반 처리를 허용한다.

```json
{
  "sourceType": "localPath",
  "path": "D:/media/sample.mp4"
}
```

장점:

* 대용량 이미지/동영상 처리에 유리하다.
* 불필요한 업로드 과정이 없다.
* OpenCV에서 직접 파일을 읽을 수 있다.

---

## 9.2 LAN Web UI 파일 처리

모바일/브라우저에서는 로컬 경로를 직접 사용할 수 없다.
따라서 다음 방식 중 하나를 사용한다.

### 방식 A: 서버 PC의 기존 파일 선택

외부 클라이언트는 서버 PC에 등록된 작업 파일 목록에서 선택한다.

```txt
Server Library
├─ imported-images
├─ imported-videos
└─ recent-files
```

### 방식 B: 브라우저 업로드

외부 클라이언트가 파일을 업로드한다.

```txt
Mobile Browser
↓
Upload
↓
Server Temp Directory
↓
C++ OpenCV Processing
↓
Result Preview / Download
```

### 방식 C: Desktop에서 파일 선택 후 모바일은 보기만 허용

가장 안전한 기본 방식이다.

```txt
Desktop에서 파일 열기
↓
Backend가 작업 관리
↓
Mobile은 상태/결과 확인
```

---

## 10. 주요 화면 구성

## 10.1 Dashboard

역할:

* 전체 시스템 상태 확인
* 백엔드 상태 확인
* LAN Web UI 접속 상태 표시
* 현재 실행 중인 작업 표시
* 최근 처리 결과 표시

구성:

```txt
Dashboard
├─ Backend Health
├─ LAN Web UI Status
├─ Connected Clients
├─ Current Jobs
├─ Recent Results
├─ Processing Time Chart
└─ Error Summary
```

---

## 10.2 Remote Access 화면

역할:

LAN Web UI Mode를 제어한다.

구성:

```txt
Remote Access
├─ Enable LAN Web UI
├─ Current Host IP
├─ UI Port
├─ API Port
├─ WebSocket Port
├─ Access PIN
├─ QR Code
├─ Session Timeout
├─ Connected Clients
├─ Read-only / Control Mode
└─ Disconnect All
```

---

## 10.3 Image Lab

역할:

이미지 파일을 열고 OpenCV 기반 처리를 실험한다.

기능:

```txt
- 이미지 열기
- 원본/결과 비교
- Zoom / Pan
- Resize
- Crop
- Rotate
- Flip
- Grayscale
- Blur
- Gaussian Blur
- Canny Edge
- Threshold
- Contour Detection
- Histogram Equalization
- 결과 저장
```

Desktop에서는 파일 경로 기반 처리를 기본으로 한다.
LAN Web UI에서는 업로드 또는 서버 라이브러리 선택 방식을 사용한다.

---

## 10.4 Video Lab

역할:

동영상 파일을 프레임 단위로 분석하고 처리한다.

기능:

```txt
- 동영상 열기
- 재생 / 일시정지
- 프레임 단위 이동
- 특정 구간 선택
- 프레임 추출
- 필터 적용
- Motion Detection
- Background Subtraction
- Object Tracking
- 결과 영상 저장
```

주의:

* 대용량 영상 처리는 Desktop에서 실행하는 것을 우선한다.
* 모바일에서는 진행률 확인과 결과 미리보기를 중심으로 제공한다.

---

## 10.5 Vision Pipeline Flow

역할:

React Flow 기반으로 OpenCV 처리 파이프라인을 구성한다.

노드 예시:

```txt
Input Node
├─ Image Input
├─ Video Input
└─ Camera Input

Processing Node
├─ Resize
├─ Grayscale
├─ Blur
├─ Threshold
├─ Canny
├─ Morphology
├─ Contour
├─ Motion Detection
└─ Object Tracking

Output Node
├─ Preview
├─ Save Image
├─ Save Video
└─ Export Metadata
```

기능:

```txt
- 노드 추가
- 노드 삭제
- 노드 연결
- 노드별 파라미터 설정
- 파이프라인 실행
- 단계별 결과 미리보기
- 실패 노드 표시
- 처리 시간 측정
- 파이프라인 JSON 저장/불러오기
```

---

## 10.6 Chart Showcase

역할:

처리 결과와 시스템 상태를 시각화한다.

차트 예시:

```txt
- 작업 처리 시간 추이
- 알고리즘별 평균 처리 시간
- 프레임별 검출 객체 수
- 작업 성공/실패 비율
- CPU / Memory 사용량
- 영상 프레임 처리 속도
```

---

## 10.7 Data Grid

역할:

작업 이력, 로그, 검출 결과, 메타데이터를 표 형태로 표시한다.

기능:

```txt
- 정렬
- 검색
- 필터
- 컬럼 표시/숨김
- CSV Export
- 행 상세 보기
- 대량 데이터 가상 스크롤
```

---

## 10.8 Logs

역할:

프론트엔드와 백엔드 로그를 사용자에게 노출한다.

기능:

```txt
- 실시간 로그 스트리밍
- 로그 레벨 필터
- 검색
- 복사
- 파일 열기
- 에러 상세 보기
- 클라이언트별 접속 로그
```

---

## 10.9 Settings

역할:

모든 설정을 UI에서 변경 가능하게 한다.

구성:

```txt
Appearance
├─ Theme Mode: Light / Dark / System
├─ Accent Color
├─ Density
└─ Font Size

Backend
├─ Max Worker Threads
├─ Temp Directory
├─ Output Directory
└─ Job Retention

Remote Access
├─ Enable LAN Web UI
├─ UI Port
├─ API Port
├─ WebSocket Port
├─ Access Mode
├─ PIN
├─ Token Expiration
└─ Allowed IPs

OpenCV
├─ Default Image Format
├─ Default Video Codec
├─ Default FPS
├─ Preview Resolution
├─ Contour Detection Defaults
└─ Enable CUDA

Logging
├─ Log Level
├─ Rolling File Size
└─ Retention Days
```

---

## 11. MUI Theme System

## 11.1 기본 원칙

```txt
- 모든 색상은 Theme Token으로 관리한다.
- 컴포넌트 내부에 색상 하드코딩을 금지한다.
- Light / Dark / System Theme을 지원한다.
- Desktop과 Mobile에서 동일 Theme을 사용한다.
- Density, Font Size, Radius, Shadow를 중앙 관리한다.
```

---

## 11.2 Theme 구조

```txt
frontend/src/theme/
├─ index.ts
├─ createAppTheme.ts
├─ tokens.ts
├─ palette.ts
├─ typography.ts
├─ shape.ts
├─ shadows.ts
├─ components.ts
└─ AppThemeProvider.tsx
```

---

## 11.3 Theme Mode

```ts
type ThemeMode = 'light' | 'dark' | 'system';
```

동작:

```txt
light  → 항상 Light Theme
dark   → 항상 Dark Theme
system → prefers-color-scheme 기준
```

---

## 11.4 Hydration Mismatch 방지 규칙

이 프로젝트는 기본적으로 Vite SPA 구조이므로 SSR 기반 hydration 문제는 적다.
그러나 향후 SSR, 정적 프리렌더링, 웹 배포 확장을 고려해 다음 규칙을 적용한다.

```txt
- 초기 렌더링 전 theme mode를 안전하게 확정한다.
- localStorage는 렌더링 본문에서 직접 호출하지 않는다.
- window, document, matchMedia는 client effect 또는 안전한 유틸에서만 사용한다.
- ThemeProvider는 앱 최상단에서 한 번만 적용한다.
- CssBaseline을 반드시 적용한다.
- system theme 변경 이벤트를 구독한다.
- theme 설정은 localStorage와 backend settings에 동기화한다.
```

---

## 12. 반응형 UI 정책

## 12.1 Desktop Layout

```txt
- Sidebar Navigation
- Top Toolbar
- Split Pane
- Multi Panel Layout
- Large Preview
- Detailed Parameter Panel
```

---

## 12.2 Tablet Layout

```txt
- Collapsible Sidebar
- Bottom Sheet Parameter Panel
- Preview 중심 구성
- Touch-friendly control
```

---

## 12.3 Mobile Layout

```txt
- Bottom Navigation
- Dashboard 중심
- 작업 상태 확인
- 결과 미리보기
- 로그 확인
- 간단한 실행/중지
```

---

## 12.4 모바일에서 제한할 기능

```txt
- 복잡한 노드 편집
- 대용량 영상 업로드
- 정밀한 타임라인 편집
- 임의 로컬 경로 저장
- 시스템 설정 변경
```

---

## 13. API 설계

## 13.1 기본 원칙

```txt
- 모든 주요 기능은 HTTP API 또는 WebSocket 이벤트로 제공한다.
- React UI는 WebView2 전용 API에 직접 의존하지 않는다.
- Desktop UI와 LAN Web UI가 같은 API를 사용한다.
- 대용량 파일은 경로 또는 업로드 세션으로 관리한다.
- 오래 걸리는 작업은 Job 기반으로 처리한다.
```

---

## 13.2 REST API

```txt
System
GET    /api/health
GET    /api/server-info
GET    /api/network-info

Settings
GET    /api/settings
PUT    /api/settings

Remote Access
GET    /api/remote/status
POST   /api/remote/enable
POST   /api/remote/disable
POST   /api/remote/rotate-token
GET    /api/remote/clients
POST   /api/remote/disconnect-all

Files
POST   /api/files/open-local
POST   /api/files/upload
GET    /api/files/library
DELETE /api/files/{fileId}

Images
POST   /api/images/process
POST   /api/images/save
GET    /api/images/results/{resultId}

Videos
POST   /api/videos/process
POST   /api/videos/export
GET    /api/videos/frame/{jobId}/{frameIndex}

Pipelines
POST   /api/pipelines/execute
GET    /api/pipelines
POST   /api/pipelines
PUT    /api/pipelines/{pipelineId}
DELETE /api/pipelines/{pipelineId}

Jobs
GET    /api/jobs
GET    /api/jobs/{jobId}
POST   /api/jobs/{jobId}/cancel
DELETE /api/jobs/{jobId}

Logs
GET    /api/logs/recent
GET    /api/logs/download
```

---

## 13.3 WebSocket Events

```txt
job.started
job.progress
job.completed
job.failed
job.cancelled

log.appended

backend.status.changed

remote.client.connected
remote.client.disconnected

preview.image.updated
preview.frame.updated

pipeline.node.started
pipeline.node.completed
pipeline.node.failed
```

---

## 14. 상태 동기화 정책

여러 클라이언트가 동시에 접속할 수 있으므로 상태 동기화가 중요하다.

## 14.1 서버가 소유하는 상태

```txt
- 현재 작업 목록
- 작업 진행률
- 처리 결과
- 로그
- 연결된 클라이언트
- Remote Access 상태
- OpenCV 처리 상태
```

---

## 14.2 클라이언트가 소유하는 상태

```txt
- 현재 선택한 화면
- 현재 열린 패널
- UI density
- 임시 파이프라인 편집 상태
- 로컬 필터 조건
```

---

## 14.3 동기화 원칙

```txt
- 작업 상태는 서버가 authoritative source이다.
- WebSocket으로 상태 변경 이벤트를 모든 클라이언트에 브로드캐스트한다.
- 클라이언트는 이벤트 수신 후 TanStack Query cache를 갱신한다.
- 충돌 가능성이 있는 설정 변경은 서버에서 검증한다.
```

---

## 15. Frontend 폴더 구조

```txt
frontend/
├─ src/
│  ├─ app/
│  │  ├─ App.tsx
│  │  ├─ router.tsx
│  │  └─ providers.tsx
│  │
│  ├─ theme/
│  │  ├─ index.ts
│  │  ├─ createAppTheme.ts
│  │  ├─ tokens.ts
│  │  ├─ components.ts
│  │  └─ AppThemeProvider.tsx
│  │
│  ├─ api/
│  │  ├─ client.ts
│  │  ├─ wsClient.ts
│  │  ├─ imageApi.ts
│  │  ├─ videoApi.ts
│  │  ├─ pipelineApi.ts
│  │  ├─ remoteApi.ts
│  │  └─ settingsApi.ts
│  │
│  ├─ runtime/
│  │  ├─ runtimeMode.ts
│  │  ├─ desktopAdapter.ts
│  │  ├─ lanWebAdapter.ts
│  │  └─ fileAdapter.ts
│  │
│  ├─ store/
│  │  ├─ useAppStore.ts
│  │  ├─ useThemeStore.ts
│  │  ├─ usePipelineStore.ts
│  │  └─ useRemoteAccessStore.ts
│  │
│  ├─ features/
│  │  ├─ dashboard/
│  │  ├─ remote-access/
│  │  ├─ image-lab/
│  │  ├─ video-lab/
│  │  ├─ pipeline-flow/
│  │  ├─ chart-showcase/
│  │  ├─ data-grid/
│  │  ├─ logs/
│  │  └─ settings/
│  │
│  ├─ shared/
│  │  ├─ components/
│  │  ├─ hooks/
│  │  ├─ layouts/
│  │  ├─ utils/
│  │  └─ types/
│  │
│  └─ main.tsx
│
├─ package.json
├─ vite.config.ts
└─ tsconfig.json
```

---

## 16. Backend 폴더 구조

```txt
backend/
├─ CMakeLists.txt
├─ src/
│  ├─ main.cpp
│  │
│  ├─ host/
│  │  ├─ DesktopHost.cpp
│  │  ├─ WebViewHost.cpp
│  │  └─ TrayHost.cpp
│  │
│  ├─ server/
│  │  ├─ StaticFileServer.cpp
│  │  ├─ ApiServer.cpp
│  │  ├─ WebSocketServer.cpp
│  │  ├─ NetworkInfo.cpp
│  │  └─ RemoteAccessManager.cpp
│  │
│  ├─ security/
│  │  ├─ AccessToken.cpp
│  │  ├─ PinAuth.cpp
│  │  ├─ SessionManager.cpp
│  │  └─ ClientRegistry.cpp
│  │
│  ├─ image/
│  │  ├─ ImageService.cpp
│  │  ├─ ImageFilters.cpp
│  │  └─ ImageResultStore.cpp
│  │
│  ├─ video/
│  │  ├─ VideoService.cpp
│  │  ├─ VideoReader.cpp
│  │  ├─ VideoWriter.cpp
│  │  └─ FrameProcessor.cpp
│  │
│  ├─ vision/
│  │  ├─ EdgeDetection.cpp
│  │  ├─ ContourDetection.cpp
│  │  ├─ MotionDetection.cpp
│  │  ├─ ObjectTracking.cpp
│  │  └─ PipelineExecutor.cpp
│  │
│  ├─ jobs/
│  │  ├─ JobQueue.cpp
│  │  ├─ JobStatus.cpp
│  │  └─ JobEvents.cpp
│  │
│  ├─ storage/
│  │  ├─ Database.cpp
│  │  ├─ SettingsRepository.cpp
│  │  ├─ JobRepository.cpp
│  │  └─ FileLibraryRepository.cpp
│  │
│  ├─ logging/
│  │  ├─ Logger.cpp
│  │  └─ LogStreamer.cpp
│  │
│  └─ common/
│     ├─ Result.h
│     ├─ ErrorCode.h
│     ├─ JsonUtils.h
│     ├─ FileUtils.h
│     └─ PathUtils.h
│
└─ tests/
```

백엔드 구현 규칙:

```txt
- main.cpp는 composition root로만 사용한다.
- REST route 등록은 server/ApiServer.*에 둔다.
- WebSocket 런타임은 server/WebSocketGateway.*에 둔다.
- EventHub, JobQueue, LogStore, RemoteAccessManager, ImageResultStore처럼 상태를 소유하는 책임은 클래스 단위로 분리한다.
- OpenCV 알고리즘 구현은 image, video, vision 폴더에 둔다.
- 상속, 인터페이스, 소유 관계가 생기면 해당 header에서 참조가 보이도록 선언한다.
```

---

## 17. OpenCV 기능 로드맵

## Phase A: 이미지 기본 처리

```txt
- 이미지 열기
- 이미지 저장
- Resize
- Crop
- Rotate
- Flip
- Grayscale
- Blur
- Gaussian Blur
- Median Blur
- Canny Edge
- Threshold
- Histogram Equalization
```

---

## Phase B: 동영상 기본 처리

```txt
- 동영상 열기
- FPS / 해상도 / 코덱 정보 확인
- 프레임 추출
- 프레임 단위 이동
- 구간 선택
- 프레임별 필터 적용
- 결과 영상 저장
```

---

## Phase C: 컴퓨터비전 기법

```txt
- Contour Detection
- Hough Line
- Hough Circle
- Background Subtraction
- Motion Detection
- Feature Matching
- Template Matching
- Object Tracking
```

---

## Phase D: 노드 기반 파이프라인

```txt
- React Flow 노드 정의
- 노드별 파라미터 패널
- 파이프라인 JSON 저장/불러오기
- C++ PipelineExecutor 구현
- 단계별 결과 캐싱
- 실패 노드 표시
- 처리 시간 프로파일링
```

---

## Phase E: 고급 기능

```txt
- Camera Input
- 실시간 프레임 처리
- ROI 지정
- Batch Processing
- CUDA 옵션 검토
- Plugin 형태 알고리즘 추가
- 원격 클라이언트별 권한 관리
```

---

## 18. 개발 단계

## Phase 0: 프로젝트 스캐폴딩

목표:

```txt
- React + Vite + TypeScript 프로젝트 생성
- MUI 적용
- C++ CMake 프로젝트 생성
- WebView2 Host 샘플 생성
- C++ 정적 파일 서버 구현
- React build 결과를 localhost에서 제공
```

완료 조건:

```txt
- Windows 앱 실행 시 WebView2에서 React UI가 표시된다.
- 브라우저에서 http://127.0.0.1:18730 접속 시 같은 UI가 표시된다.
- /api/health 응답이 정상 동작한다.
```

---

## Phase 1: MUI Theme System & App Shell

목표:

```txt
- App Shell Layout 구현
- Sidebar / Topbar / Content Layout 구성
- Light / Dark / System Theme 구현
- Hydration mismatch 방지 규칙 적용
- Settings 화면에서 테마 변경 가능
```

완료 조건:

```txt
- 테마 전환이 즉시 반영된다.
- 앱 재시작 후에도 테마 설정이 유지된다.
- Desktop과 모바일 브라우저에서 동일 Theme이 적용된다.
```

---

## Phase 2: LAN Web UI Mode

목표:

```txt
- Remote Access 화면 구현
- LAN Web UI Mode ON/OFF 구현
- 127.0.0.1 / 0.0.0.0 바인딩 모드 전환
- 노트북 IP 탐지
- 접속 URL 표시
- QR Code 표시
- PIN 또는 Token 인증 구현
```

완료 조건:

```txt
- 같은 Wi-Fi의 모바일에서 노트북 IP로 접속 가능하다.
- 인증 후 React UI가 표시된다.
- 접속 중인 클라이언트가 Remote Access 화면에 표시된다.
```

---

## Phase 3: Backend API & Logging

목표:

```txt
- REST API 기본 구조 구현
- WebSocket 서버 구현
- spdlog 파일 로그 구현
- Logs 화면 구현
- 실시간 로그 스트리밍 구현
```

완료 조건:

```txt
- UI에서 백엔드 상태 확인 가능
- 로그가 파일로 저장됨
- WebView2와 모바일 양쪽에서 로그 확인 가능
```

---

## Phase 4: Image Lab

목표:

```txt
- 이미지 열기
- 이미지 미리보기
- OpenCV 기본 필터 구현
- Before / After 비교
- 결과 저장
```

완료 조건:

```txt
- Desktop에서 로컬 이미지를 선택하고 처리 가능
- 모바일에서 결과 확인 가능
- 처리 실패 시 에러가 UI에 표시됨
```

---

## Phase 5: Video Lab

목표:

```txt
- 동영상 열기
- 프레임 정보 표시
- 프레임 추출
- 프레임 단위 필터 적용
- 결과 영상 저장
- 진행률 WebSocket 전송
```

완료 조건:

```txt
- Desktop에서 동영상을 처리할 수 있다.
- 모바일에서 진행률과 결과 미리보기를 확인할 수 있다.
```

---

## Phase 6: Pipeline Flow

목표:

```txt
- React Flow 기반 노드 에디터 구현
- OpenCV 처리 노드 정의
- C++ PipelineExecutor 구현
- 파이프라인 실행 결과 표시
```

완료 조건:

```txt
- 사용자가 노드 기반 이미지 처리 파이프라인을 만들 수 있다.
- 실행 상태가 모든 접속 클라이언트에 동기화된다.
```

---

## Phase 7: Dashboard & Showcase

목표:

```txt
- 차트 쇼케이스 구현
- 작업 통계 표시
- Data Grid 구현
- 작업 이력 표시
- 연결 클라이언트 표시
```

완료 조건:

```txt
- 작업 결과를 차트와 테이블로 확인 가능
- 여러 클라이언트에서 동일 상태 확인 가능
```

---

## Phase 8: Packaging & Template화

목표:

```txt
- Windows 앱 패키징
- 프론트엔드 빌드 자동화
- 백엔드 빌드 자동화
- WebView2 Runtime 체크
- 방화벽 예외 안내
- 템플릿 문서화
```

완료 조건:

```txt
- 새 PC에서 앱 실행 가능
- LAN Web UI Mode 사용 방법 문서화
- 개발자용 확장 가이드 작성
```

---

## 19. 개발 우선순위

최우선:

```txt
1. WebView2 + React UI 실행
2. C++ 정적 파일 서버
3. /api/health
4. MUI Theme System
5. LAN Web UI Mode 기본 구조
```

다음 우선순위:

```txt
6. QR Code 접속
7. Token/PIN 인증
8. WebSocket 로그
9. Image Lab
10. Job Queue
```

그 다음:

```txt
11. Video Lab
12. React Flow Pipeline
13. Chart Dashboard
14. Data Grid
15. Packaging
```

후순위:

```txt
16. Camera Input
17. CUDA
18. Plugin System
19. 모바일 업로드 처리
20. 고급 비디오 편집
```

---

## 20. 주의할 점

## 20.1 네트워크 주의점

```txt
- Windows 방화벽에서 포트 차단 가능성이 있다.
- LAN Web UI Mode 활성화 시 방화벽 안내가 필요하다.
- 공용 Wi-Fi에서는 사용을 제한하거나 경고해야 한다.
- 0.0.0.0 바인딩은 사용자가 명시적으로 켠 경우에만 허용한다.
```

---

## 20.2 보안 주의점

```txt
- 인증 없는 외부 접속을 허용하지 않는다.
- 모바일 클라이언트는 기본적으로 Read-only Mode로 시작한다.
- 로컬 파일 시스템 접근은 Desktop Host에서만 수행한다.
- 업로드 파일은 temp directory에 격리한다.
- 경로 traversal 공격을 방지한다.
- API 요청마다 권한을 검증한다.
```

---

## 20.3 OpenCV 주의점

```txt
- 대용량 영상 처리는 UI 스레드를 막지 않는다.
- 모든 장시간 처리는 Job Queue에서 실행한다.
- 코덱 실패 가능성을 로그로 남긴다.
- 임시 파일과 결과 파일 정리 정책을 둔다.
- 프레임 미리보기와 전체 영상 저장 처리를 분리한다.
```

---

## 20.4 React UI 주의점

```txt
- WebView2 전용 API에 직접 의존하지 않는다.
- 모든 백엔드 기능은 API client를 통해 호출한다.
- Desktop과 LAN Web UI의 차이는 runtime adapter에서 처리한다.
- 모바일 화면에서는 기능을 줄이고 상태 확인 중심으로 구성한다.
- 대용량 이미지/영상 데이터를 base64로 계속 주고받지 않는다.
```

---

## 21. 성공 기준

1차 성공 기준:

```txt
- Windows 앱에서 WebView2로 React UI가 표시된다.
- 브라우저에서 localhost로 같은 UI에 접속 가능하다.
- MUI Light / Dark / System Theme이 정상 동작한다.
- C++ /api/health가 정상 응답한다.
- LAN Web UI Mode를 켜면 모바일에서 접속 가능하다.
- QR Code 접속이 가능하다.
- PIN 또는 Token 인증이 동작한다.
```

2차 성공 기준:

```txt
- OpenCV 이미지 처리가 가능하다.
- 처리 결과를 Desktop과 모바일에서 모두 확인 가능하다.
- WebSocket으로 작업 진행률이 동기화된다.
- 로그를 UI에서 확인할 수 있다.
- Job Queue로 장시간 작업을 처리한다.
```

3차 성공 기준:

```txt
- 동영상 프레임 처리가 가능하다.
- React Flow로 OpenCV 처리 파이프라인을 구성할 수 있다.
- 차트와 테이블로 작업 결과를 시각화할 수 있다.
- 여러 클라이언트가 동일 상태를 안정적으로 공유한다.
```

---

## 22. 최종 방향성

이 프로젝트는 다음 형태를 목표로 한다.

```txt
Windows 앱이면서
동시에 로컬 네트워크 웹앱 서버이고
React 기반 공통 UI를 사용하며
C++ OpenCV 백엔드가 고성능 처리를 담당하는
범용 비전/미디어 테스트베드
```

최종 원칙:

```txt
- React UI는 공통 웹 UI로 만든다.
- WebView2는 Windows 앱에서 그 UI를 띄우는 Host일 뿐이다.
- C++ Backend는 OpenCV 처리와 API 서버 역할을 모두 담당한다.
- LAN Web UI Mode는 기본값 OFF로 두고 사용자가 켰을 때만 외부 접속을 허용한다.
- 모바일 접속은 상태 확인, 결과 미리보기, 간단한 제어 중심으로 설계한다.
- 무거운 처리와 로컬 파일 접근은 Windows 실행 PC에서 담당한다.
- 모든 설정, 로그, 에러는 UI에 노출한다.
- 향후 실제 제품으로 확장 가능한 구조를 유지한다.
```
