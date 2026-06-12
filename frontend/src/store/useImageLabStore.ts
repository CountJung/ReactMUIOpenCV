import { create } from 'zustand';
import { persist } from 'zustand/middleware';
import type { ImageOperation, ImageResult } from '../api/imageApi';

export type ImageParamValue = string | number;
export type ImageParams = Record<string, ImageParamValue>;

type ImageLabState = {
  localPath: string;
  currentResult: ImageResult | null;
  operation: ImageOperation;
  params: ImageParams;
  savePath: string | null;
  setLocalPath: (localPath: string) => void;
  setCurrentResult: (currentResult: ImageResult | null) => void;
  setOperation: (operation: ImageOperation) => void;
  setParams: (params: ImageParams) => void;
  setParam: (key: string, value: ImageParamValue) => void;
  setSavePath: (savePath: string | null) => void;
};

export const useImageLabStore = create<ImageLabState>()(
  persist(
    (set) => ({
      localPath: '',
      currentResult: null,
      operation: 'grayscale',
      params: {},
      savePath: null,
      setLocalPath: (localPath) => set({ localPath }),
      setCurrentResult: (currentResult) => set({ currentResult }),
      setOperation: (operation) => set({ operation }),
      setParams: (params) => set({ params }),
      setParam: (key, value) => set((state) => ({ params: { ...state.params, [key]: value } })),
      setSavePath: (savePath) => set({ savePath }),
    }),
    {
      name: 'react-mui-opencv-image-lab',
      partialize: (state) => ({
        localPath: state.localPath,
        currentResult: state.currentResult,
        operation: state.operation,
        params: state.params,
        savePath: state.savePath,
      }),
    },
  ),
);
