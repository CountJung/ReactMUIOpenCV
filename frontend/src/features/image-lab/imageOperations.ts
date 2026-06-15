import type { ImageOperation, ImageResult } from '../../api/imageApi';
import type { ImageParams } from '../../store/useImageLabStore';

export const operationLabels: Array<{ value: ImageOperation; label: string }> = [
  { value: 'resize', label: 'Resize' },
  { value: 'crop', label: 'Crop' },
  { value: 'rotate', label: 'Rotate' },
  { value: 'flip', label: 'Flip' },
  { value: 'grayscale', label: 'Grayscale' },
  { value: 'blur', label: 'Blur' },
  { value: 'gaussianBlur', label: 'Gaussian Blur' },
  { value: 'sharpen', label: 'Sharpen' },
  { value: 'threshold', label: 'Threshold' },
  { value: 'edgeDetect', label: 'Edge Detect' },
  { value: 'contourDetect', label: 'Contour Detect' },
  { value: 'histogram', label: 'Histogram' },
  { value: 'colorConvert', label: 'Color Convert' },
  { value: 'compare', label: 'Compare Diff' },
  { value: 'featureAlign', label: 'Feature Align' },
  { value: 'eccAlign', label: 'ECC Align' },
  { value: 'qrScan', label: 'QR Scanner' },
  { value: 'calibrationBoard', label: 'Calibration Board' },
  { value: 'blobCentroid', label: 'Blob Centroid' },
  { value: 'convexHull', label: 'Convex Hull' },
  { value: 'huMoments', label: 'Hu Moments' },
  { value: 'houghTransform', label: 'Hough Transform' },
  { value: 'inpaint', label: 'Inpaint' },
  { value: 'seamlessClone', label: 'Seamless Clone' },
  { value: 'alphaBlend', label: 'Alpha Blend' },
  { value: 'exposureFusion', label: 'Exposure Fusion' },
  { value: 'hdrTonemap', label: 'HDR Tonemap' },
  { value: 'stylization', label: 'Stylization' },
  { value: 'pencilSketch', label: 'Pencil Sketch' },
];

export const alignmentUtilityOperations: ImageOperation[] = ['featureAlign', 'eccAlign', 'qrScan'];
export const shapeAnalysisOperations: ImageOperation[] = [
  'blobCentroid',
  'convexHull',
  'huMoments',
  'houghTransform',
];
export const advancedRenderOperations: ImageOperation[] = [
  'inpaint',
  'seamlessClone',
  'alphaBlend',
  'exposureFusion',
  'hdrTonemap',
  'stylization',
  'pencilSketch',
];

export function labelForOperation(operation: ImageOperation) {
  return operationLabels.find((item) => item.value === operation)?.label ?? operation;
}

export function defaultParams(operation: ImageOperation, result?: ImageResult): ImageParams {
  const width = result?.width ?? 640;
  const height = result?.height ?? 480;
  const roiWidth = Math.max(8, Math.floor(width * 0.35));
  const roiHeight = Math.max(8, Math.floor(height * 0.35));

  switch (operation) {
    case 'resize':
      return { width: result?.width ?? 1280, height: result?.height ?? 720 };
    case 'crop':
      return {
        x: 0,
        y: 0,
        width: Math.max(1, Math.floor(width * 0.75)),
        height: Math.max(1, Math.floor(height * 0.75)),
      };
    case 'rotate':
      return { angle: 90 };
    case 'flip':
      return { direction: 'horizontal' };
    case 'blur':
    case 'gaussianBlur':
      return { kernel: 7 };
    case 'sharpen':
      return { strength: 1 };
    case 'threshold':
      return { threshold: 128 };
    case 'edgeDetect':
    case 'contourDetect':
      return { low: 80, high: 160 };
    case 'colorConvert':
      return { target: 'hsv' };
    case 'featureAlign':
      return { maxFeatures: 500, keepRatio: 0.18 };
    case 'eccAlign':
      return { iterations: 80, epsilon: 0.0001 };
    case 'calibrationBoard':
      return { boardWidth: 9, boardHeight: 6, squareSize: 1 };
    case 'blobCentroid':
      return { threshold: 128, polarity: 'dark', minArea: 80, maxShapes: 24 };
    case 'convexHull':
      return { threshold: 128, polarity: 'dark', minArea: 80, maxShapes: 16 };
    case 'huMoments':
      return { threshold: 128, polarity: 'dark', minArea: 80, maxShapes: 8 };
    case 'houghTransform':
      return {
        mode: 'lines',
        low: 80,
        high: 160,
        threshold: 80,
        minLineLength: 40,
        maxLineGap: 12,
        maxShapes: 32,
      };
    case 'inpaint':
      return { maskMode: 'edges', threshold: 220, radius: 3, width: roiWidth, height: roiHeight };
    case 'seamlessClone':
      return {
        x: Math.max(0, Math.floor((width - roiWidth) / 2)),
        y: Math.max(0, Math.floor((height - roiHeight) / 2)),
        width: roiWidth,
        height: roiHeight,
        targetX: Math.floor(width * 0.62),
        targetY: Math.floor(height * 0.55),
        mode: 'mixed',
      };
    case 'alphaBlend':
      return { alpha: 0.5 };
    case 'exposureFusion':
      return { darkGamma: 1.8, brightGamma: 0.55, contrast: 1, saturation: 1, exposure: 0 };
    case 'hdrTonemap':
      return { exposureScale: 2.2, gamma: 1, intensity: 0, lightAdapt: 0.8, colorAdapt: 0 };
    case 'stylization':
      return { sigmaS: 60, sigmaR: 0.45 };
    case 'pencilSketch':
      return { mode: 'color', sigmaS: 60, sigmaR: 0.07, shade: 0.02 };
    default:
      return {};
  }
}
