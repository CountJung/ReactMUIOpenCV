import { openLocalImage, uploadImageFile, type ImageResult } from '../api/imageApi';
import type { RuntimeMode } from './runtimeMode';

export type ImageOpenCapabilities = {
  localPath: boolean;
  upload: boolean;
};

export function getImageOpenCapabilities(runtimeMode: RuntimeMode): ImageOpenCapabilities {
  return {
    localPath: runtimeMode === 'desktop',
    upload: true,
  };
}

export function openImageFromLocalPath(path: string, runtimeMode: RuntimeMode): Promise<ImageResult> {
  if (!getImageOpenCapabilities(runtimeMode).localPath) {
    return Promise.reject(new Error('Local image paths are only available in desktop mode.'));
  }

  return openLocalImage(path);
}

export function openImageFromUpload(file: File): Promise<ImageResult> {
  return uploadImageFile(file);
}
