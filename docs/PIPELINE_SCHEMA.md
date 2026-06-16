# Pipeline JSON Schema

Phase 6 uses one JSON document shape across the React Flow editor and the C++ `PipelineExecutor`. Phase 9B extends the same schema with video input nodes for motion-analysis pipelines. Phase 9C adds ROI object tracking through `trackObject` operation nodes. Phase 9E adds reusable shape-analysis image operations: `blobCentroid`, `convexHull`, `huMoments`, and `houghTransform`. Phase 9F adds advanced image composition and rendering operations such as `inpaint`, `seamlessClone`, `alphaBlend`, `exposureFusion`, `hdrTonemap`, `stylization`, and `pencilSketch`. README showcase generation uses the same operation node shape through `visionSampleBoard`. Phase 9G adds optional DNN operations that run only when model assets are present under `models/dnn`. Phase 9I adds coordinate-based `perspectiveExtract` for reusing Contour Extractor selections in pipelines.

```json
{
  "version": 1,
  "name": "Image Pipeline",
  "nodes": [
    {
      "id": "input-1",
      "type": "imageInput",
      "position": { "x": 0, "y": 120 },
      "data": {
        "label": "Image Input",
        "resultId": "existing-image-result-id"
      }
    },
    {
      "id": "operation-1",
      "type": "operation",
      "position": { "x": 260, "y": 120 },
      "data": {
        "label": "Grayscale",
        "operation": "grayscale",
        "params": {}
      }
    },
    {
      "id": "output-1",
      "type": "output",
      "position": { "x": 520, "y": 120 },
      "data": {
        "label": "Preview Output"
      }
    }
  ],
  "edges": [
    { "id": "input-1-operation-1", "source": "input-1", "target": "operation-1" },
    { "id": "operation-1-output-1", "source": "operation-1", "target": "output-1" }
  ]
}
```

## Node Types

- `imageInput`: Starts an image pipeline from an existing Image Lab `resultId`.
- `videoInput`: Starts a video pipeline from an existing Video Lab `videoId`.
- `operation`: Applies an OpenCV image operation, or a video operation when upstream is `videoInput`. Image operation names match `frontend/src/api/imageApi.ts`; video operations currently include `opticalFlow`, `stabilize`, and `trackObject`.
- `output`: Marks the final preview result or video pipeline completion.

## Video Tracking Operation

`trackObject` uses the same pipeline node shape as other operations and accepts:

```json
{
  "operation": "trackObject",
  "params": {
    "startFrame": 0,
    "endFrame": 120,
    "roi": { "x": 0, "y": 0, "width": 160, "height": 120 }
  }
}
```

The backend clamps the ROI to the selected start frame and stores per-frame bounding boxes, match scores, and lost-frame flags in `data/video-tracking.json`.

## Shape Analysis Operations

Shape-analysis operations use the normal image operation node shape and produce Image Lab results with overlay previews plus `metadata.shape` records for Data Grid inspection.

```json
{
  "operation": "blobCentroid",
  "params": {
    "threshold": 128,
    "polarity": "dark",
    "minArea": 80,
    "maxShapes": 24
  }
}
```

`convexHull` and `huMoments` use the same threshold/polarity/min-area/max-shapes parameters. `polarity: "dark"` treats dark shapes on a light background as foreground; `polarity: "light"` treats bright shapes as foreground. `houghTransform` accepts `mode: "lines"` with `minLineLength` and `maxLineGap`, or `mode: "circles"` with `minDist`, `accumulator`, `minRadius`, and `maxRadius`.

## Advanced Rendering Operations

Advanced Image Lab rendering operations also use normal image operation nodes and produce cached Image Lab results. `inpaint` accepts mask presets such as `edges`, `bright`, `dark`, or `center`; `seamlessClone` accepts a source ROI and target center; `alphaBlend` blends the current result against the original image; `exposureFusion` synthesizes exposure variants; `hdrTonemap` applies a Reinhard tonemap preview; `stylization` and `pencilSketch` use OpenCV non-photorealistic rendering filters.

## README Sample Board Operation

`visionSampleBoard` creates a single Image Lab result containing original, CLAHE, Canny edge overlay, adaptive threshold, contour overlay, and ORB feature-point panels. If `tileWidth` or `tileHeight` is omitted, the backend chooses a default tile size from the input image aspect ratio.

```json
{
  "operation": "visionSampleBoard",
  "params": {
    "tileWidth": 350,
    "tileHeight": 460
  }
}
```

The result metadata includes `metadata.sampleBoard.contourCount` and `metadata.sampleBoard.orbKeypoints`.

## Perspective Extract Operation

`perspectiveExtract` creates an independent Image Lab result by warping four selected contour points into a rectangular output. Points are image-space pixels ordered approximately top-left, top-right, bottom-right, bottom-left; the backend reorders defensively before warping.

```json
{
  "operation": "perspectiveExtract",
  "params": {
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

The result metadata includes `metadata.contourExtraction.candidateId`, `sourcePoints`, `outputSize`, `area`, and `boundingBox`.

## Optional DNN Operations

DNN operations use the normal image operation node shape and require model files stored below `models/dnn`. The backend accepts relative paths only and rejects absolute paths or `..` traversal. Available operations are `dnnFaceDetection`, `dnnYoloDetection`, `dnnTextDetection`, `dnnPoseEstimation`, and `dnnMaskRcnn`.

```json
{
  "operation": "dnnYoloDetection",
  "params": {
    "modelPath": "yolo/yolov8n.onnx",
    "labelsPath": "yolo/coco.names",
    "inputWidth": 640,
    "inputHeight": 640,
    "confidenceThreshold": 0.35,
    "nmsThreshold": 0.45,
    "scale": 0.0039215686,
    "swapRB": 1
  }
}
```

Model discovery is available through `GET /api/dnn/models`; Image Lab shows the discovered relative paths in the Optional DNN Examples card. Public model packages are listed through `GET /api/dnn/catalog` and downloaded through `POST /api/dnn/download` with `{ "packageId": "yolo11n-coco" }`. The backend only downloads fixed catalog URLs into `models/dnn`; arbitrary URL download is intentionally not supported.

## Execution

`POST /api/pipelines/execute` accepts `{ "document": PipelineDocument }`.

The backend executes a single linear path from the first `imageInput` or `videoInput` node through outgoing edges. Image operations produce new Image Lab results, so intermediate steps are cached by `ImageResultStore` and can be previewed through normal image preview URLs. Shape-analysis image operations also attach structured metadata to those Image Lab result records. Video motion operations record metrics such as tracked features, average flow magnitude, stabilization crop estimate, and processing time. Tracking operations record per-frame ROI metadata for Data Grid inspection.
