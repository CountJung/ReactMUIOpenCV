import { apiRequest } from './client';

export type RemoteClient = {
  id: string;
  address: string;
  permission: 'read-only' | 'control';
  connectedAt: string;
  expiresAt: string;
};

export type RemoteStatus = {
  enabled: boolean;
  lanBound: boolean;
  bindHost: string;
  selectedIp: string;
  port: number;
  localUrl: string;
  lanUrl: string;
  connectionUrl: string;
  accessToken: string;
  pin: string;
  sessionTimeoutMinutes: number;
  defaultPermission: 'read-only';
  clients: RemoteClient[];
  message?: string;
};

export type NetworkInfo = {
  hostName: string;
  bindHost: string;
  port: number;
  lanBound: boolean;
  selectedIp: string;
  addresses: string[];
};

export type RemoteAuthResult = {
  sessionToken: string;
  permission: 'read-only' | 'control';
  expiresAt: string;
};

export function getNetworkInfo() {
  return apiRequest<NetworkInfo>('/api/network-info');
}

export function getRemoteStatus() {
  return apiRequest<RemoteStatus>('/api/remote/status');
}

export function enableRemoteAccess() {
  return apiRequest<RemoteStatus>('/api/remote/enable', { method: 'POST' });
}

export function disableRemoteAccess() {
  return apiRequest<RemoteStatus>('/api/remote/disable', { method: 'POST' });
}

export function rotateRemoteToken() {
  return apiRequest<RemoteStatus>('/api/remote/rotate-token', { method: 'POST' });
}

export function disconnectAllRemoteClients() {
  return apiRequest<{ clients: RemoteClient[]; message: string }>('/api/remote/disconnect-all', {
    method: 'POST',
  });
}

export function authenticateRemoteClient(credentials: { token?: string; pin?: string }) {
  return apiRequest<RemoteAuthResult>('/api/remote/auth', {
    method: 'POST',
    body: JSON.stringify(credentials),
  });
}
