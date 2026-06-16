import type { ThemeMode } from '../theme/themeMode';
import { apiRequest } from './client';

export type ContourDetectionSettings = {
  maxCandidates: number;
  low: number;
  high: number;
  minAreaRatio: number;
  epsilon: number;
  retrieval: 'list' | 'external';
};

export type PerformanceBenchmarkSettings = {
  defaultIterations: number;
  maxIterations: number;
};

export type LoggingSettings = {
  level: 'debug' | 'info' | 'warning' | 'error';
  maxFileMb: number;
  retentionDays: number;
};

export type AppSettings = {
  themeMode: ThemeMode;
  opencv: {
    contourDetection: ContourDetectionSettings;
    performanceBenchmark: PerformanceBenchmarkSettings;
  };
  logging: LoggingSettings;
};

export type AppSettingsPatch = {
  themeMode?: ThemeMode;
  opencv?: {
    contourDetection?: ContourDetectionSettings;
    performanceBenchmark?: PerformanceBenchmarkSettings;
  };
  logging?: LoggingSettings;
};

export async function getSettings(): Promise<AppSettings | null> {
  return apiRequest<AppSettings>('/api/settings');
}

export async function updateSettings(settings: AppSettingsPatch): Promise<AppSettings> {
  return apiRequest<AppSettings>('/api/settings', {
    method: 'PUT',
    body: JSON.stringify(settings),
  });
}
