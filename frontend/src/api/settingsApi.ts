import type { ThemeMode } from '../theme/themeMode';
import { apiRequest } from './client';

export type AppSettings = {
  themeMode: ThemeMode;
};

export async function getSettings(): Promise<AppSettings | null> {
  return apiRequest<AppSettings>('/api/settings');
}

export async function updateSettings(settings: AppSettings): Promise<AppSettings> {
  return apiRequest<AppSettings>('/api/settings', {
    method: 'PUT',
    body: JSON.stringify(settings),
  });
}
