# Pipeline JSON Schema

Phase 6 uses one JSON document shape across the React Flow editor and the C++ `PipelineExecutor`. Phase 9B extends the same schema with video input nodes for motion-analysis pipelines. Phase 9C adds ROI object tracking through `trackObject` operation nodes. Phase 9E adds reusable shape-analysis image operations: `blobCentroid`, `convexHull`, `huMoments`, and `houghTransform`. Phase 9F adds advanced image composition and rendering operations such as `inpaint`, `seamlessClone`, `alphaBlend`, `exposureFusion`, `hdrTonemap`, `stylization`, and `pencilSketch`.

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

## Execution

`POST /api/pipelines/execute` accepts `{ "document": PipelineDocument }`.

The backend executes a single linear path from the first `imageInput` or `videoInput` node through outgoing edges. Image operations produce new Image Lab results, so intermediate steps are cached by `ImageResultStore` and can be previewed through normal image preview URLs. Shape-analysis image operations also attach structured metadata to those Image Lab result records. Video motion operations record metrics such as tracked features, average flow magnitude, stabilization crop estimate, and processing time. Tracking operations record per-frame ROI metadata for Data Grid inspection.
