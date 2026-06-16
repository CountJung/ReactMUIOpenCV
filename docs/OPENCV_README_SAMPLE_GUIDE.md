# README OpenCV 샘플 생성 가이드

이 문서는 `README.md` 첫 화면의 `docs/assets/readme/opencv-vision-sample.png`와 같은 형태의 이미지를 이 프로젝트 앱에서 직접 생성하는 방법을 설명합니다.

핵심 흐름은 간단합니다.

```txt
Image Lab에서 이미지 열기
→ Vision Sample Board 선택
→ Apply
→ Save PNG
```

## 생성되는 결과

`Vision Sample Board` 연산은 현재 Image Lab 결과 하나를 입력으로 받아 2x3 비교판을 생성합니다.

```txt
Original input       Grayscale + CLAHE     Canny edge map
Adaptive threshold   Contour overlay       ORB feature points
```

백엔드는 이 비교판을 C++ OpenCV 처리 결과로 저장합니다. 결과 metadata에는 contour 수와 ORB keypoint 수가 함께 들어가며, UI의 `README Sample Board` 카드에서 확인할 수 있습니다.

## 준비

Release 빌드가 없다면 저장소 루트에서 실행합니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1
```

앱 모드로 실행합니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\start.ps1 -Mode App -Release
```

브라우저 웹 모드로 실행해도 같은 UI를 사용할 수 있습니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\start.ps1 -Mode Web -Release
```

## 샘플 이미지로 따라 하기

1. 왼쪽 메뉴에서 `Image Lab`으로 이동합니다.
2. `Source` 카드의 `Local Image Path`에 입력 이미지 경로를 넣습니다.

```txt
D:\ArielNetworks\Media\ChatGPT Image 2025년 3월 31일 오후 05_40_00.png
```

3. `Open Path`를 누릅니다.
4. `README Sample Board` 카드에서 `Vision Sample Board` 칩을 누릅니다.
5. `Operation` 카드의 `Filter` 값이 `Vision Sample Board`로 바뀐 것을 확인합니다.
6. `Tile W`, `Tile H`는 열린 이미지의 가로/세로 비율에 맞춰 자동으로 채워집니다. 다른 비율의 이미지를 올려도 보드가 자연스럽게 나오며, 필요하면 값을 직접 조정합니다.
7. `Apply`를 누릅니다.
8. 오른쪽 또는 아래쪽 `After` 미리보기에서 2x3 비교판을 확인합니다.
9. 파일로 저장하려면 `Save PNG`를 누릅니다.

저장 파일은 기본적으로 다음 폴더 아래에 만들어집니다.

```txt
outputs/images/
```

## README 이미지를 교체하는 방법

`Save PNG`로 저장한 결과를 README 첫 화면에 쓰려면 파일을 아래 경로에 복사하고 기존 파일을 교체합니다.

```txt
docs/assets/readme/opencv-vision-sample.png
```

README는 이 상대 경로를 참조합니다.

```markdown
![OpenCV vision processing sample](docs/assets/readme/opencv-vision-sample.png)
```

GitHub는 저장소 안의 상대 경로 이미지를 README에서 바로 렌더링합니다.

## Pipeline Flow에서 재사용하기

같은 샘플 보드 생성을 반복해서 써야 하면 Pipeline Flow에서 노드로 구성할 수 있습니다.

1. `Pipeline Flow`로 이동합니다.
2. `Image Input` 노드에 Image Lab에서 만든 원본 `resultId`를 지정합니다.
3. `Operation` 노드의 operation을 `Vision Sample Board`로 선택합니다.
4. params를 비워두면 백엔드가 입력 이미지 비율로 타일 크기를 자동 산정합니다. 직접 고정하려면 다음처럼 지정합니다.

```json
{
  "tileWidth": 350,
  "tileHeight": 460
}
```

5. 파이프라인을 실행하면 Image Lab 결과와 동일한 형식의 보드 이미지가 생성됩니다.

## 내부 처리 단계

`Vision Sample Board`는 `backend/src/image/ImageFilters.cpp`의 Image Lab 연산으로 구현되어 있습니다.

1. 입력 이미지를 README에 맞는 세로형 타일 크기로 crop/resize합니다.
2. 원본 타일을 첫 번째 칸에 배치합니다.
3. CLAHE로 grayscale 대비를 높입니다.
4. Gaussian blur 후 `Canny` edge map을 만들고 원본 위에 노란 선으로 overlay합니다.
5. adaptive threshold 결과를 흑백 스케치처럼 표시합니다.
6. edge map에서 contour를 찾고 주요 contour와 bounding box를 overlay합니다.
7. `ORB` feature point를 검출해 원본 위에 원형 keypoint로 표시합니다.
8. 여섯 결과를 2x3 보드로 합쳐 Image Lab 결과로 저장합니다.

## 문제 해결

- `Open Path`가 비활성화되어 있으면 LAN 모드 또는 브라우저 업로드 흐름일 수 있습니다. 앱 모드에서 실행하거나 `Upload`로 이미지를 올립니다.
- 결과가 README와 다르게 보이면 원본 이미지에서 바로 `Vision Sample Board`를 실행했는지 확인합니다. 이미 필터가 적용된 결과에서 실행하면 그 결과를 다시 입력으로 사용합니다.
- `Tile W`, `Tile H`를 바꾸면 전체 보드 크기와 crop 구성이 달라집니다. 여러 이미지 비율을 비교하려면 먼저 자동값을 사용하고, 같은 크기로 맞춰 비교해야 할 때만 수동으로 고정합니다.
- 이미 올린 이미지는 `Image Lab`의 `Uploaded / Opened Images` 목록에서 다시 선택할 수 있습니다. 이미 올린 동영상도 `Video Lab`의 `Uploaded / Opened Videos` 목록에서 다시 선택할 수 있습니다.
- 저장 위치가 궁금하면 `Save PNG` 후 표시되는 성공 메시지의 경로를 확인합니다.
