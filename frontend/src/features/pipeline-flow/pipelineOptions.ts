import type { PipelineDocument, PipelineNodeType, PipelineOperation } from '../../api/pipelineApi';

export const nodeKindOptions: Array<{ value: PipelineNodeType; label: string }> = [
  { value: 'imageInput', label: 'Image Input' },
  { value: 'videoInput', label: 'Video Input' },
  { value: 'operation', label: 'Operation' },
  { value: 'output', label: 'Output' },
];

export const operationOptions: Array<{
  value: PipelineOperation;
  label: string;
  defaultParams?: Record<string, unknown>;
}> = [
  { value: 'grayscale', label: 'Grayscale' },
  { value: 'blur', label: 'Blur', defaultParams: { kernel: 7 } },
  { value: 'gaussianBlur', label: 'Gaussian Blur', defaultParams: { kernel: 7 } },
  { value: 'threshold', label: 'Threshold', defaultParams: { threshold: 128 } },
  { value: 'edgeDetect', label: 'Edge Detect', defaultParams: { low: 80, high: 160 } },
  { value: 'contourDetect', label: 'Contour Detect', defaultParams: { low: 80, high: 160 } },
  { value: 'histogram', label: 'Histogram' },
  { value: 'sharpen', label: 'Sharpen', defaultParams: { strength: 1 } },
  {
    value: 'blobCentroid',
    label: 'Blob Centroid',
    defaultParams: { threshold: 128, polarity: 'dark', minArea: 80, maxShapes: 24 },
  },
  {
    value: 'convexHull',
    label: 'Convex Hull',
    defaultParams: { threshold: 128, polarity: 'dark', minArea: 80, maxShapes: 16 },
  },
  {
    value: 'huMoments',
    label: 'Hu Moments',
    defaultParams: { threshold: 128, polarity: 'dark', minArea: 80, maxShapes: 8 },
  },
  {
    value: 'houghTransform',
    label: 'Hough Transform',
    defaultParams: {
      mode: 'lines',
      low: 80,
      high: 160,
      threshold: 80,
      minLineLength: 40,
      maxLineGap: 12,
      maxShapes: 32,
    },
  },
  { value: 'opticalFlow', label: 'Optical Flow', defaultParams: { sampleFrames: 120 } },
  { value: 'stabilize', label: 'Video Stabilize', defaultParams: { sampleFrames: 120 } },
  {
    value: 'trackObject',
    label: 'Track Object ROI',
    defaultParams: {
      startFrame: 0,
      endFrame: 120,
      roi: { x: 0, y: 0, width: 160, height: 120 },
    },
  },
];

export function defaultDocument(resultId = ''): PipelineDocument {
  return {
    version: 1,
    name: 'Image Pipeline',
    nodes: [
      {
        id: 'input-1',
        type: 'imageInput',
        position: { x: 0, y: 120 },
        data: { label: 'Image Input', resultId },
      },
      {
        id: 'operation-1',
        type: 'operation',
        position: { x: 260, y: 120 },
        data: { label: 'Grayscale', operation: 'grayscale', params: {} },
      },
      {
        id: 'output-1',
        type: 'output',
        position: { x: 520, y: 120 },
        data: { label: 'Preview Output' },
      },
    ],
    edges: [
      {
        id: 'input-1-operation-1',
        source: 'input-1',
        target: 'operation-1',
        sourceHandle: 'right-source',
        targetHandle: 'left-source',
      },
      {
        id: 'operation-1-output-1',
        source: 'operation-1',
        target: 'output-1',
        sourceHandle: 'right-source',
        targetHandle: 'left-source',
      },
    ],
  };
}
