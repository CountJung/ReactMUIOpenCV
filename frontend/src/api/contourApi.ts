import { API_BASE_URL, apiRequest } from './client';
import type { ImageResult, ProcessImageResponse } from './imageApi';
import type { ContourDetectionSettings } from './settingsApi';

export type ContourPoint = {
  x: number;
  y: number;
};

export type ContourCandidate = {
  candidateId: string;
  index: number;
  points: ContourPoint[];
  boundingBox: {
    x: number;
    y: number;
    width: number;
    height: number;
  };
  area: number;
  areaRatio: number;
  perimeter: number;
  score: number;
  source?: string;
  sourceIndex?: number;
};

export type ContourDetectionParams = ContourDetectionSettings;

export type ContourCandidateResponse = {
  source: ImageResult;
  method: {
    edge: string;
    retrieval: string;
    minAreaRatio: number;
    contourCount?: number;
  };
  candidates: ContourCandidate[];
};

export type ContourOcrLine = {
  text: string;
  confidence: number;
  boundingBox: {
    x: number;
    y: number;
    width: number;
    height: number;
  };
};

export type ContourOcrResponse = {
  sourceResultId: string;
  candidateId: string;
  text: string;
  confidence: number;
  lineCount: number;
  wordCount: number;
  imageSize: {
    width: number;
    height: number;
  };
  method: {
    engine: string;
    languages: string[];
    language: string;
    preprocessing: string;
    pageSegMode: string;
    tessdataPath: string;
    note?: string;
  };
  lines: ContourOcrLine[];
};

export type ContourOcrLanguage = {
  code: string;
  label: string;
};

export type ContourOcrLanguageResponse = {
  tessdataPath: string;
  tessdataAvailable: boolean;
  availableLanguages: ContourOcrLanguage[];
  defaultLanguages: string[];
  recommendedLanguages: string[][];
};

export function getContourOcrLanguages() {
  return apiRequest<ContourOcrLanguageResponse>('/api/contours/ocr/languages');
}

export function getContourCandidates(resultId: string, params: ContourDetectionParams) {
  const search = new URLSearchParams({
    resultId,
    maxCandidates: String(params.maxCandidates),
  });
  search.set('low', String(params.low));
  search.set('high', String(params.high));
  search.set('minAreaRatio', String(params.minAreaRatio));
  search.set('epsilon', String(params.epsilon));
  search.set('retrieval', params.retrieval);
  return apiRequest<ContourCandidateResponse>(`/api/contours/candidates?${search.toString()}`);
}

export async function previewContourCandidate(request: {
  resultId: string;
  candidate: ContourCandidate;
}) {
  const response = await fetch(`${API_BASE_URL}/api/contours/preview`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(request),
  });

  if (!response.ok) {
    const body = (await response.json().catch(() => null)) as null | {
      error?: { message?: string };
    };
    throw new Error(body?.error?.message ?? `Contour preview failed: ${response.status}`);
  }

  return response.blob();
}

export function recognizeContourCandidateText(request: {
  resultId: string;
  candidate: ContourCandidate;
  languages: string[];
  pageSegMode: string;
}) {
  return apiRequest<ContourOcrResponse>('/api/contours/ocr', {
    method: 'POST',
    body: JSON.stringify(request),
  });
}

export function extractContourCandidate(request: {
  resultId: string;
  candidate: ContourCandidate;
}) {
  return apiRequest<ProcessImageResponse>('/api/contours/extract', {
    method: 'POST',
    body: JSON.stringify(request),
  });
}
