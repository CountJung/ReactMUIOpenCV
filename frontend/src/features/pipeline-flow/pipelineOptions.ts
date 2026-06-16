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
  { value: 'inpaint', label: 'Inpaint', defaultParams: { maskMode: 'edges', radius: 3 } },
  {
    value: 'seamlessClone',
    label: 'Seamless Clone',
    defaultParams: {
      x: 0,
      y: 0,
      width: 160,
      height: 120,
      targetX: 240,
      targetY: 180,
      mode: 'mixed',
    },
  },
  { value: 'alphaBlend', label: 'Alpha Blend', defaultParams: { alpha: 0.5 } },
  {
    value: 'exposureFusion',
    label: 'Exposure Fusion',
    defaultParams: { darkGamma: 1.8, brightGamma: 0.55, contrast: 1, saturation: 1, exposure: 0 },
  },
  {
    value: 'hdrTonemap',
    label: 'HDR Tonemap',
    defaultParams: { exposureScale: 2.2, gamma: 1, intensity: 0, lightAdapt: 0.8, colorAdapt: 0 },
  },
  { value: 'stylization', label: 'Stylization', defaultParams: { sigmaS: 60, sigmaR: 0.45 } },
  {
    value: 'pencilSketch',
    label: 'Pencil Sketch',
    defaultParams: { mode: 'color', sigmaS: 60, sigmaR: 0.07, shade: 0.02 },
  },
  {
    value: 'visionSampleBoard',
    label: 'Vision Sample Board',
    defaultParams: { tileWidth: 350, tileHeight: 460 },
  },
  {
    value: 'dnnFaceDetection',
    label: 'DNN Face Detection',
    defaultParams: {
      modelPath: '',
      configPath: '',
      inputWidth: 300,
      inputHeight: 300,
      confidenceThreshold: 0.5,
      scale: 1,
      meanB: 104,
      meanG: 177,
      meanR: 123,
      swapRB: 0,
    },
  },
  {
    value: 'dnnYoloDetection',
    label: 'DNN YOLO Detection',
    defaultParams: {
      modelPath: '',
      labelsPath: '',
      inputWidth: 640,
      inputHeight: 640,
      confidenceThreshold: 0.35,
      nmsThreshold: 0.45,
      scale: 0.0039215686,
      meanB: 0,
      meanG: 0,
      meanR: 0,
      swapRB: 1,
    },
  },
  {
    value: 'dnnTextDetection',
    label: 'DNN Text Detection',
    defaultParams: {
      modelPath: '',
      inputWidth: 320,
      inputHeight: 320,
      confidenceThreshold: 0.5,
    },
  },
  {
    value: 'dnnPoseEstimation',
    label: 'DNN Pose Estimation',
    defaultParams: {
      modelPath: '',
      configPath: '',
      inputWidth: 368,
      inputHeight: 368,
      confidenceThreshold: 0.2,
      scale: 0.0039215686,
      meanB: 0,
      meanG: 0,
      meanR: 0,
      swapRB: 1,
    },
  },
  {
    value: 'dnnMaskRcnn',
    label: 'DNN Mask R-CNN',
    defaultParams: {
      modelPath: '',
      configPath: '',
      labelsPath: '',
      inputWidth: 800,
      inputHeight: 800,
      confidenceThreshold: 0.5,
      maskThreshold: 0.3,
      scale: 0.0039215686,
      meanB: 0,
      meanG: 0,
      meanR: 0,
      swapRB: 1,
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
