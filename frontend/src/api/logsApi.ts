import { apiRequest } from './client';

export type LogEntry = {
  id: string;
  timestamp: string;
  level: 'info' | 'warning' | 'error' | string;
  message: string;
};

export function getRecentLogs() {
  return apiRequest<{ logs: LogEntry[] }>('/api/logs/recent');
}
