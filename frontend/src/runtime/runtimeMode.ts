export type RuntimeMode = 'desktop' | 'lan';

export function getRuntimeMode(): RuntimeMode {
  return window.location.hostname === '127.0.0.1' || window.location.hostname === 'localhost'
    ? 'desktop'
    : 'lan';
}
