import { apiRequest } from './client';

export type FileLibraryItem = {
  id: string;
  name: string;
  kind: string;
  sizeBytes?: number;
  sourceType?: string;
  createdAt?: string;
};

export function getFileLibrary() {
  return apiRequest<{ files: FileLibraryItem[] }>('/api/files/library');
}
