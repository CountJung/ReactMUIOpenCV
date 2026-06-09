import { openLocalImage, uploadImageFile, type ImageResult } from '../api/imageApi';
import { openLocalVideo, uploadVideoFile, type VideoRecord } from '../api/videoApi';
import type { RuntimeMode } from './runtimeMode';

export type ImageOpenCapabilities = {
  localPath: boolean;
  upload: boolean;
};

export type VideoOpenCapabilities = ImageOpenCapabilities;

export function getImageOpenCapabilities(runtimeMode: RuntimeMode): ImageOpenCapabilities {
  return {
    localPath: runtimeMode === 'desktop',
    upload: true,
  };
}

export function getVideoOpenCapabilities(runtimeMode: RuntimeMode): VideoOpenCapabilities {
  return getImageOpenCapabilities(runtimeMode);
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

export function openVideoFromLocalPath(path: string, runtimeMode: RuntimeMode): Promise<VideoRecord> {
  if (!getVideoOpenCapabilities(runtimeMode).localPath) {
    return Promise.reject(new Error('Local video paths are only available in desktop mode.'));
  }

  return openLocalVideo(path);
}

export function openVideoFromUpload(file: File): Promise<VideoRecord> {
  return uploadVideoFile(file);
}
