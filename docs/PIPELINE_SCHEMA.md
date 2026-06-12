# Pipeline JSON Schema

Phase 6 uses one JSON document shape across the React Flow editor and the C++ `PipelineExecutor`. Phase 9B extends the same schema with video input nodes for motion-analysis pipelines.

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
- `operation`: Applies an OpenCV image operation, or a video motion operation when upstream is `videoInput`. Image operation names match `frontend/src/api/imageApi.ts`; video motion operations currently include `opticalFlow` and `stabilize`.
- `output`: Marks the final preview result or video pipeline completion.

## Execution

`POST /api/pipelines/execute` accepts `{ "document": PipelineDocument }`.

The backend executes a single linear path from the first `imageInput` or `videoInput` node through outgoing edges. Image operations produce new Image Lab results, so intermediate steps are cached by `ImageResultStore` and can be previewed through normal image preview URLs. Video motion operations record metrics such as tracked features, average flow magnitude, stabilization crop estimate, and processing time.
