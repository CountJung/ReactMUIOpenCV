# OpenCV Performance Benchmark Guide

이 문서는 Pack 9H 성능 계측 샘플을 앱에서 재현하는 방법을 정리한다. 현재 샘플은 같은 Image Lab 결과를 대상으로 세 가지 픽셀 반전 구현을 반복 실행하고, 처리량과 평균 시간을 기록한다.

## 측정 대상

- Manual pointer loop: `cv::Mat::ptr<cv::Vec3b>`로 행/열을 직접 순회한다.
- OpenCV `forEach`: `cv::Mat::forEach<cv::Vec3b>`로 픽셀 루프를 OpenCV 실행 모델에 맡긴다.
- OpenCV `bitwise_not`: 기존 OpenCV 내장 필터 스타일의 기준 구현으로 비교한다.

## Performance Lab에서 실행하기

1. Image Lab에서 이미지를 열거나 업로드한다.
2. 필요한 경우 기본 필터나 `Vision Sample Board`를 먼저 실행해 비교하고 싶은 Image Result를 만든다.
3. 왼쪽 내비게이션에서 `Performance Lab`으로 이동한다.
4. Image Result에서 측정할 이미지를 선택하거나, 같은 화면에서 `Open` 또는 `Upload`로 새 이미지를 추가한다.
5. Iterations를 1에서 20 사이로 지정한다. 기본값은 5다.
6. `Run Benchmark`를 누르면 `performance-benchmark` job이 생성되고 완료 후 결과가 저장된다.
7. 하단 `Benchmark History`에서 이전 샘플을 다시 선택해 비교하거나 delete/clear 버튼으로 기록을 정리한다.

## 결과 확인

- Dashboard의 `Benchmarks` 카드에서 전체 샘플 수, 최신 fastest method, `forEach` 대비 포인터 루프 배율을 확인한다.
- Performance Lab의 `Comparison` 카드에서 선택한 샘플의 method별 MP/s 처리량과 평균 시간을 비교한다.
- Charts의 `Pixel Benchmarks` 카드에서도 최신 샘플의 method별 MP/s 처리량을 확인할 수 있다.
- Data Grid의 `Benchmarks` 탭에서 각 샘플의 이미지 크기, 반복 횟수, method별 평균 ms와 MP/s를 정렬/검색할 수 있다.
- Data Grid는 benchmark를 실행하는 화면이 아니라 저장된 record를 검증하는 화면이다. `Benchmarks` 탭의 `Run` 버튼으로 Performance Lab에 이동해 샘플을 생성한 뒤 다시 돌아와 표를 확인한다.

## 저장 위치와 API

벤치마크 기록은 백엔드가 소유하며 `data/performance-benchmarks.json`에 저장된다. 로컬 루프백 클라이언트에서는 storage path도 조회된다.

- `GET /api/performance/benchmarks`: 저장된 benchmark record 목록 조회
- `POST /api/performance/benchmarks/run`: `{ "resultId": "...", "iterations": 5 }`로 새 job 실행 및 기록
- `DELETE /api/performance/benchmarks/{benchmarkId}`: 단일 benchmark record 삭제
- `DELETE /api/performance/benchmarks`: benchmark history 전체 삭제

주의: 수치는 이미지 크기, OpenCV 빌드 옵션, CPU 스레딩 설정, 현재 시스템 부하에 따라 달라진다. 여러 번 실행해 경향을 비교하는 용도로 사용한다.
