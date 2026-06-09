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
  | 'compare';

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

export function getImageResults() {
  return apiRequest<{ results: ImageResult[] }>('/api/images/results');
}
