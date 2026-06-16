import { API_BASE_URL, apiRequest } from './client';

export type ImageOperation =
  | 'open'
  | 'resize'
  | 'crop'
  | 'rotate'
  | 'flip'
  | 'grayscale'
  | 'blur'
  | 'gaussianBlur'
  | 'sharpen'
  | 'threshold'
  | 'edgeDetect'
  | 'contourDetect'
  | 'histogram'
  | 'colorConvert'
  | 'compare'
  | 'featureAlign'
  | 'eccAlign'
  | 'qrScan'
  | 'calibrationBoard'
  | 'blobCentroid'
  | 'convexHull'
  | 'huMoments'
  | 'houghTransform'
  | 'inpaint'
  | 'seamlessClone'
  | 'alphaBlend'
  | 'exposureFusion'
  | 'hdrTonemap'
  | 'stylization'
  | 'pencilSketch'
  | 'visionSampleBoard';

export type ShapeAnalysisMetadata = {
  operation: 'blobCentroid' | 'convexHull' | 'huMoments' | 'houghTransform' | string;
  contourCount?: number;
  shapeCount?: number;
  largestArea?: number;
  lineCount?: number;
  circleCount?: number;
  mode?: string;
  blobs?: unknown[];
  hulls?: unknown[];
  shapes?: unknown[];
  lines?: unknown[];
  circles?: unknown[];
};

export type ImageResult = {
  resultId: string;
  name: string;
  sourceType: 'localPath' | 'upload';
  sourcePath: string;
  operation: ImageOperation;
  params: Record<string, unknown>;
  width: number;
  height: number;
  channels: number;
  createdAt: string;
  previewUrl: string;
  originalPreviewUrl: string;
  metadata?: {
    shape?: ShapeAnalysisMetadata;
    composition?: {
      operation?: string;
      [key: string]: unknown;
    };
    sampleBoard?: {
      operation?: string;
      tileWidth?: number;
      tileHeight?: number;
      contourCount?: number;
      orbKeypoints?: number;
      stages?: string[];
    };
    [key: string]: unknown;
  };
};

export type ProcessImageResponse = {
  job: {
    id: string;
    type: string;
    status: string;
    progress: number;
    message: string;
    createdAt: string;
    updatedAt: string;
  };
  result: ImageResult;
};

export type SaveImageResponse = {
  resultId: string;
  status: 'saved';
  path: string;
};

export type CalibrationResult = {
  calibrationId: string;
  imageResultId: string;
  imageName: string;
  imageSize: { width: number; height: number };
  board: { width: number; height: number; squareSize: number };
  cornerCount: number;
  rmsReprojectionError: number;
  cameraMatrix: number[][];
  distortionCoefficients: number[];
  rotationVector: number[];
  translationVector: number[];
  createdAt: string;
};

export function absoluteImageUrl(path: string) {
  return `${API_BASE_URL}${path}`;
}

export function openLocalImage(path: string) {
  return apiRequest<ImageResult>('/api/images/open-local', {
    method: 'POST',
    body: JSON.stringify({ path }),
  });
}

export async function uploadImageFile(file: File) {
  const formData = new FormData();
  formData.append('file', file);

  const response = await fetch(`${API_BASE_URL}/api/images/upload`, {
    method: 'POST',
    body: formData,
  });

  const body = (await response.json()) as {
    ok: boolean;
    data: ImageResult | null;
    error: null | { message: string };
  };

  if (!response.ok || !body.ok || !body.data) {
    throw new Error(body.error?.message ?? `Image upload failed: ${response.status}`);
  }

  return body.data;
}

export function processImage(request: {
  resultId: string;
  operation: ImageOperation;
  params?: Record<string, unknown>;
}) {
  return apiRequest<ProcessImageResponse>('/api/images/process', {
    method: 'POST',
    body: JSON.stringify(request),
  });
}

export function saveImageResult(resultId: string, format: 'png' | 'jpg' = 'png') {
  return apiRequest<SaveImageResponse>('/api/images/save', {
    method: 'POST',
    body: JSON.stringify({ resultId, format }),
  });
}

export function getImageResult(resultId: string) {
  return apiRequest<ImageResult>(`/api/images/results/${resultId}`);
}

export function deleteImageResult(resultId: string) {
  return apiRequest<{ deleted: string }>(`/api/images/results/${resultId}`, {
    method: 'DELETE',
  });
}

export function getImageResults() {
  return apiRequest<{ results: ImageResult[] }>('/api/images/results');
}

export function getCalibrationResults() {
  return apiRequest<{
    records: CalibrationResult[];
    storage: { kind: string; description: string; path?: string };
  }>('/api/calibration/results');
}

export function createCalibrationResult(request: {
  resultId: string;
  boardWidth: number;
  boardHeight: number;
  squareSize: number;
}) {
  return apiRequest<{ calibration: CalibrationResult }>('/api/calibration/results', {
    method: 'POST',
    body: JSON.stringify(request),
  });
}
