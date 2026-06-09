import { apiRequest } from './client';

export type JobRecord = {
  id: string;
  type: string;
  status: 'queued' | 'running' | 'completed' | 'failed' | 'cancelled' | string;
  progress: number;
  message: string;
  createdAt: string;
  updatedAt: string;
};

export function getJobs() {
  return apiRequest<{ jobs: JobRecord[] }>('/api/jobs');
}
