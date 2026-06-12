import { API_BASE_URL, apiRequest } from './client';
import type { JobRecord } from './jobsApi';

export type VideoFilter = 'none' | 'grayscale' | 'blur' | 'edgeDetect' | 'threshold' | 'opticalFlow' | 'stabilize';

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

export type VideoDiagnostics = {
  video: VideoRecord;
  operation?: string;
  sampleFrames: number;
  framesRead: number;
  elapsedMs: number;
  metadataFps: number;
  measuredReadFps: number;
  trackedFeatures?: number;
  averageFlowMagnitude?: number;
  stabilizationCropPercent?: number;
  processingMs?: number;
  previewFrameUrl?: string;
  displayFrameUrl?: string;
  writeContainer?: string;
  writeCodec?: string;
  record?: VideoDiagnosticsRecord;
};

export type VideoDiagnosticsRecord = {
  diagnosticId: string;
  videoId: string;
  videoName: string;
  sourceType: string;
  width: number;
  height: number;
  frameCount: number;
  durationSeconds: number;
  sampleFrames: number;
  framesRead: number;
  elapsedMs: number;
  metadataFps: number;
  measuredReadFps: number;
  operation?: string;
  trackedFeatures?: number;
  averageFlowMagnitude?: number;
  stabilizationCropPercent?: number;
  processingMs?: number;
  writeContainer?: string;
  writeCodec?: string;
  createdAt: string;
};

export type VideoMotionMetrics = VideoDiagnostics & {
  operation: 'opticalFlow' | 'stabilize' | string;
  trackedFeatures: number;
  averageFlowMagnitude: number;
  stabilizationCropPercent: number;
  processingMs: number;
  previewFrameUrl: string;
};

export type TrackingRoi = {
  x: number;
  y: number;
  width: number;
  height: number;
};

export type VideoTrackingFrame = TrackingRoi & {
  frameIndex: number;
  score: number;
  lost: boolean;
};

export type VideoTrackingRecord = {
  trackingId: string;
  videoId: string;
  videoName: string;
  width: number;
  height: number;
  frameCount: number;
  method: string;
  status: string;
  startFrame: number;
  endFrame: number;
  framesTracked: number;
  averageScore: number;
  processingMs: number;
  sourceRoi: TrackingRoi;
  frames: VideoTrackingFrame[];
  createdAt: string;
};

export type VideoTrackingResult = {
  video: VideoRecord;
  method: string;
  status: string;
  startFrame: number;
  endFrame: number;
  sourceRoi: TrackingRoi;
  framesTracked: number;
  averageScore: number;
  processingMs: number;
  frames: VideoTrackingFrame[];
  record?: VideoTrackingRecord;
};

export type VideoDiagnosticsList = {
  records: VideoDiagnosticsRecord[];
  storage: {
    kind: string;
    description: string;
    path?: string;
  };
};

export type VideoTrackingList = {
  records: VideoTrackingRecord[];
  storage: {
    kind: string;
    description: string;
    path?: string;
  };
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

export function getVideoDiagnostics(videoId: string, sampleFrames = 120) {
  const search = new URLSearchParams({ sampleFrames: String(sampleFrames) });
  return apiRequest<VideoDiagnostics>(`/api/videos/${videoId}/diagnostics?${search.toString()}`);
}

export function analyzeVideoMotion(request: {
  videoId: string;
  operation: 'opticalFlow' | 'stabilize';
  sampleFrames?: number;
}) {
  return apiRequest<{ job: JobRecord; metrics: VideoMotionMetrics }>('/api/videos/motion-metrics', {
    method: 'POST',
    body: JSON.stringify(request),
  });
}

export function trackVideo(request: {
  videoId: string;
  startFrame: number;
  endFrame: number;
  roi: TrackingRoi;
}) {
  return apiRequest<{ job: JobRecord; tracking: VideoTrackingResult }>('/api/videos/track', {
    method: 'POST',
    body: JSON.stringify(request),
  });
}

export function getVideoDiagnosticsHistory() {
  return apiRequest<VideoDiagnosticsList>('/api/videos/diagnostics');
}

export function getVideoTrackingHistory() {
  return apiRequest<VideoTrackingList>('/api/videos/tracking');
}

export function deleteVideo(videoId: string) {
  return apiRequest<{ deleted: string }>(`/api/videos/${videoId}`, {
    method: 'DELETE',
  });
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
