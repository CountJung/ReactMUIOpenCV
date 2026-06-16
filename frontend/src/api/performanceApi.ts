import { apiRequest } from './client';
import type { JobRecord } from './jobsApi';

export type PerformanceBenchmarkMethod = {
  name: string;
  label: string;
  totalMs: number;
  averageMs: number;
  megapixelsPerSecond: number;
  checksum: number;
};

export type PerformanceBenchmarkRecord = {
  benchmarkId: string;
  operation: string;
  imageResultId: string;
  imageName: string;
  width: number;
  height: number;
  channels: number;
  iterations: number;
  pixelCount: number;
  methods: PerformanceBenchmarkMethod[];
  fastestMethod: string;
  forEachSpeedupVsPointer: number;
  createdAt: string;
};

export type PerformanceBenchmarkList = {
  records: PerformanceBenchmarkRecord[];
  storage: {
    kind: string;
    description: string;
    path?: string;
  };
};

export function getPerformanceBenchmarks() {
  return apiRequest<PerformanceBenchmarkList>('/api/performance/benchmarks');
}

export function runPerformanceBenchmark(request: { resultId: string; iterations?: number }) {
  return apiRequest<{ job: JobRecord; benchmark: PerformanceBenchmarkRecord }>(
    '/api/performance/benchmarks/run',
    {
      method: 'POST',
      body: JSON.stringify(request),
    },
  );
}

export function deletePerformanceBenchmark(benchmarkId: string) {
  return apiRequest<{ deleted: string }>(`/api/performance/benchmarks/${benchmarkId}`, {
    method: 'DELETE',
  });
}

export function clearPerformanceBenchmarks() {
  return apiRequest<{ deleted: number }>('/api/performance/benchmarks', {
    method: 'DELETE',
  });
}
