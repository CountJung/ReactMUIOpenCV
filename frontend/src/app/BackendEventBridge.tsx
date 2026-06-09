import { useQueryClient } from '@tanstack/react-query';
import { useEffect } from 'react';
import { createWsClient, type BackendEvent } from '../api/wsClient';

const eventInvalidations: Record<string, string[][]> = {
  'job.started': [['jobs']],
  'job.progress': [['jobs']],
  'job.completed': [['jobs']],
  'job.failed': [['jobs']],
  'job.cancelled': [['jobs']],
  'log.appended': [['logs']],
  'backend.status.changed': [['remote-status'], ['network-info']],
  'remote.client.connected': [['remote-status']],
  'remote.client.disconnected': [['remote-status']],
  'preview.image.updated': [['image-results']],
  'preview.frame.updated': [['video-frames'], ['video-library']],
  'pipeline.node.started': [['jobs'], ['pipelines']],
  'pipeline.node.completed': [['jobs'], ['pipelines']],
  'pipeline.node.failed': [['jobs'], ['pipelines']],
};

function parseBackendEvent(data: string): BackendEvent | null {
  try {
    const parsed = JSON.parse(data) as unknown;
    if (typeof parsed === 'object' && parsed !== null && 'type' in parsed) {
      return parsed as BackendEvent;
    }
  } catch {
    return null;
  }

  return null;
}

export function BackendEventBridge() {
  const queryClient = useQueryClient();

  useEffect(() => {
    const socket = createWsClient();

    socket.addEventListener('message', (message) => {
      if (typeof message.data !== 'string') {
        return;
      }

      const event = parseBackendEvent(message.data);
      if (!event) {
        return;
      }

      const queryKeys = eventInvalidations[event.type] ?? [];
      for (const queryKey of queryKeys) {
        void queryClient.invalidateQueries({ queryKey });
      }
    });

    return () => socket.close();
  }, [queryClient]);

  return null;
}
