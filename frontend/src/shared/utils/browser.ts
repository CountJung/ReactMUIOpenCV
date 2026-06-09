export function isBrowser() {
  return typeof window !== 'undefined';
}

export function getLocalStorageValue(key: string) {
  if (!isBrowser()) {
    return null;
  }

  try {
    return window.localStorage.getItem(key);
  } catch {
    return null;
  }
}

export function setLocalStorageValue(key: string, value: string) {
  if (!isBrowser()) {
    return;
  }

  try {
    window.localStorage.setItem(key, value);
  } catch {
    // Ignore storage failures from private mode or locked-down WebView profiles.
  }
}

export function getSystemPrefersDark() {
  if (!isBrowser() || !window.matchMedia) {
    return false;
  }

  return window.matchMedia('(prefers-color-scheme: dark)').matches;
}
