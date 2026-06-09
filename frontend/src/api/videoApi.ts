import { API_BASE_URL, apiRequest } from './client';
import type { JobRecord } from './jobsApi';

export type VideoFilter = 'none' | 'grayscale' | 'blur' | 'edgeDetect' | 'threshold';

export type VideoRecord = {
  videoId: string;
  name: string;
  sourceType: string;
  path: string;
  width: number;
  height: number;
  fps: number;
  frameCount: number;
  durationSeconds: number;
  createdAt: string;
};

export type VideoFrameResult = {
  videoId: string;
  frameIndex: number;
  filter: VideoFilter | string;
  path: string;
  previewUrl: string;
};

export type VideoExportResult = {
  videoId: string;
  filter: VideoFilter | string;
  startFrame: number;
  endFrame: number;
  path: string;
  codec: string;
};

export function absoluteVideoUrl(path: string) {
  if (path.startsWith('http://') || path.startsWith('https://')) {
    return path;
  }

  return `${API_BASE_URL}${path}`;
}

export function videoFrameUrl(videoId: string, frameIndex: number, filter: VideoFilter, cacheKey?: string | number) {
  const search = new URLSearchParams({ filter });
  if (cacheKey !== undefined) {
    search.set('t', String(cacheKey));
  }

  return absoluteVideoUrl(`/api/videos/frame/${videoId}/${frameIndex}?${search.toString()}`);
}

export function getVideos() {
  return apiRequest<{ videos: VideoRecord[] }>('/api/videos');
}

export function getVideo(videoId: string) {
  return apiRequest<VideoRecord>(`/api/videos/${videoId}`);
}

export function openLocalVideo(path: string) {
  return apiRequest<VideoRecord>('/api/videos/open-local', {
    method: 'POST',
    body: JSON.stringify({ path }),
  });
}

export async function uploadVideoFile(file: File) {
  const formData = new FormData();
  formData.append('file', file);

  const response = await fetch(`${API_BASE_URL}/api/videos/upload`, {
    method: 'POST',
    body: formData,
  });
  const body = (await response.json()) as {
    ok: boolean;
    data: VideoRecord;
    error: null | { message: string };
  };

  if (!response.ok || !body.ok) {
    throw new Error(body.error?.message ?? `Video upload failed: ${response.status}`);
  }

  return body.data;
}

export function processVideo(request: {
  videoId: string;
  filter: VideoFilter;
  startFrame?: number;
  endFrame?: number;
}) {
  return apiRequest<{ job: JobRecord; video: VideoRecord; filter: VideoFilter }>('/api/videos/process', {
    method: 'POST',
    body: JSON.stringify(request),
  });
}

export function extractVideoFrame(request: { videoId: string; frameIndex: number; filter: VideoFilter }) {
  return apiRequest<{ job: JobRecord; result: VideoFrameResult }>('/api/videos/extract-frame', {
    method: 'POST',
    body: JSON.stringify(request),
  });
}

export function exportVideo(request: {
  videoId: string;
  filter: VideoFilter;
  startFrame: number;
  endFrame: number;
}) {
  return apiRequest<{ job: JobRecord; result: VideoExportResult }>('/api/videos/export', {
    method: 'POST',
    body: JSON.stringify(request),
  });
}
