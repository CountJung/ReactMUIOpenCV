import { getLocalStorageValue, setLocalStorageValue } from '../shared/utils/browser';

export type ThemeMode = 'light' | 'dark' | 'system';

const themeModeKey = 'react-mui-opencv.theme-mode';
const themeModes: readonly ThemeMode[] = ['light', 'dark', 'system'];

export function isThemeMode(value: string | null): value is ThemeMode {
  return themeModes.includes(value as ThemeMode);
}

export function readStoredThemeMode(): ThemeMode {
  const stored = getLocalStorageValue(themeModeKey);
  return isThemeMode(stored) ? stored : 'system';
}

export function writeStoredThemeMode(mode: ThemeMode) {
  setLocalStorageValue(themeModeKey, mode);
}
