# Pipeline JSON Schema

Phase 6 uses one JSON document shape across the React Flow editor and the C++ `PipelineExecutor`.

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
- `operation`: Applies an OpenCV image operation. Supported operation names match `frontend/src/api/imageApi.ts`.
- `output`: Marks the final preview result.

## Execution

`POST /api/pipelines/execute` accepts `{ "document": PipelineDocument }`.

The backend executes a single linear path from the first `imageInput` node through outgoing edges. Each operation produces a new Image Lab result, so intermediate steps are cached by `ImageResultStore` and can be previewed through normal image preview URLs.
