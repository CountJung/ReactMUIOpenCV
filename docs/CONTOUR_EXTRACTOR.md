# Contour Extractor Guide

Pack 9I는 문서, 라벨, 부품처럼 사각형에 가까운 대상을 이미지에서 고르고 평면 보정 결과를 독립 Image Result로 만드는 예제다.

## UI 실행 흐름

1. 왼쪽 내비게이션에서 `Contour Extractor`를 연다.
2. 기존 Image Result를 선택하거나, 같은 화면에서 로컬 경로 `Open` 또는 이미지 `Upload`로 새 입력을 만든다.
3. `Detection Parameters`는 Settings에 저장된 OpenCV contour 기본값으로 초기화된다. 필요한 경우 Canny threshold, 최소 영역 비율, 최대 후보 수, contour retrieval 방식을 이 화면에서 임시 조정한다.
4. `Detect`를 누르면 원본 이미지는 그대로 유지되고, 백엔드가 Canny edge, contour approximation, rotated-rectangle fallback, quadrilateral ranking으로 후보들을 찾는다.
5. 원본 이미지 위 후보 polygon 또는 오른쪽 후보 버튼을 선택하면 하단 `Candidate Preview`에 정면 시점 perspective preview가 표시된다.
6. `Extract`를 누르면 선택 후보만 `perspectiveExtract` Image Result로 저장되고, `Extraction History`에 누적된다.
7. 하단 `Save Latest Extracted PNG` 또는 히스토리 행의 save 버튼을 누르면 정면뷰로 추출된 결과 이미지를 별도 PNG 파일로 저장한다.
8. 후보 목록은 `Clear`로 비울 수 있고, 저장된 추출 결과는 히스토리 행의 delete 버튼으로 삭제할 수 있다.

## 결과와 재사용

- 추출 결과는 Image Lab 결과 라이브러리에 일반 Image Result로 남으며, 컨투어 페이지의 히스토리에서도 다시 선택해 볼 수 있다.
- save 버튼은 기존 Image Lab 저장 API를 사용하므로 저장 성공 시 백엔드가 반환한 파일 경로가 화면에 표시된다.
- Data Grid의 Image Results 탭에서 후보 ID, 출력 크기, 선택 영역 bounding box를 볼 수 있다.
- Pipeline Flow의 operation 목록에서 `Perspective Extract`를 선택하면 같은 좌표 기반 warp를 자동화 노드로 재사용할 수 있다.

## 기본값 설정

- Contour Extractor의 기본 검출값은 `Settings > OpenCV Contour Defaults`에서 저장한다.
- 저장된 값은 백엔드가 `data/settings.json`에 유지한다.
- Contour Extractor 화면의 `Reset`은 현재 Settings 기본값으로 되돌린다.

## API

- `GET /api/contours/candidates?resultId={id}&maxCandidates=24&low=45&high=150&minAreaRatio=0.002&epsilon=0.024&retrieval=list`
- `POST /api/contours/preview`
- `POST /api/contours/extract`

`preview`와 `extract` 요청 본문은 같은 후보 형태를 사용한다.

```json
{
  "resultId": "image-result-id",
  "candidate": {
    "candidateId": "candidate-1",
    "points": [
      { "x": 10, "y": 20 },
      { "x": 300, "y": 18 },
      { "x": 310, "y": 240 },
      { "x": 12, "y": 250 }
    ]
  }
}
```
