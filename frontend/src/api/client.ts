export const API_BASE_URL = import.meta.env.VITE_API_BASE_URL ?? 'http://127.0.0.1:18730';

type ApiEnvelope<T> = {
  ok: boolean;
  data: T;
  error: null | {
    code: string;
    message: string;
  };
};

function isApiEnvelope<T>(value: unknown): value is ApiEnvelope<T> {
  return typeof value === 'object' && value !== null && 'ok' in value && 'data' in value && 'error' in value;
}

export async function apiRequest<T>(path: string, init?: RequestInit) {
  const response = await fetch(`${API_BASE_URL}${path}`, {
    headers: {
      'Content-Type': 'application/json',
      ...init?.headers,
    },
    ...init,
  });

  const body = (await response.json()) as unknown;

  if (isApiEnvelope<T>(body)) {
    if (!body.ok) {
      throw new Error(body.error?.message ?? `API request failed: ${response.status}`);
    }

    return body.data;
  }

  if (!response.ok) {
    throw new Error(`API request failed: ${response.status}`);
  }

  return body as T;
}

export async function getHealth() {
  return apiRequest<{
    status: string;
    service: string;
    opencvVersion?: string;
  }>('/api/health');
}
