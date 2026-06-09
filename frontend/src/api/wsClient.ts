export type BackendEvent = {
  id?: string;
  type: string;
  timestamp?: string;
  payload?: unknown;
};

export function createWsClient(path = '/ws') {
  const configuredUrl = import.meta.env.VITE_WS_BASE_URL;
  if (configuredUrl) {
    return new WebSocket(configuredUrl);
  }

  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
  return new WebSocket(`${protocol}//${window.location.hostname}:18731${path}`);
}
