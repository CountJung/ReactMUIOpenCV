import type { Edge, Node, XYPosition } from '@xyflow/react';
import type { ImageOperation, ImageResult } from './imageApi';
import { apiRequest } from './client';
import type { JobRecord } from './jobsApi';
import type { VideoRecord, VideoMotionMetrics } from './videoApi';

export type VideoPipelineOperation = 'opticalFlow' | 'stabilize';
export type PipelineOperation = ImageOperation | VideoPipelineOperation;
export type PipelineNodeType = 'imageInput' | 'videoInput' | 'operation' | 'output';

export type PipelineNodeData = Record<string, unknown> & {
  label: string;
  resultId?: string;
  videoId?: string;
  operation?: PipelineOperation;
  params?: Record<string, unknown>;
};

export type PipelineFlowNode = Node<PipelineNodeData, PipelineNodeType>;

export type PipelineDocument = {
  version: 1;
  name: string;
  nodes: Array<{
    id: string;
    type: PipelineNodeType;
    position: XYPosition;
    data: PipelineNodeData;
  }>;
  edges: Array<Pick<Edge, 'id' | 'source' | 'target' | 'sourceHandle' | 'targetHandle'>>;
};

export type PipelineRecord = {
  id: string;
  document: PipelineDocument;
  updatedAt: string;
};

export type PipelineStep = {
  nodeId: string;
  nodeType: PipelineNodeType;
  status: 'completed' | 'failed' | string;
  operation?: PipelineOperation;
  params?: Record<string, unknown>;
  result?: ImageResult;
  video?: VideoRecord;
  metrics?: VideoMotionMetrics;
  timestamp: string;
};

export type PipelineExecution = {
  executionId: string;
  job: JobRecord;
  status: 'completed' | 'failed' | string;
  steps: PipelineStep[];
  result?: ImageResult;
  error?: string;
  completedAt: string;
};

export type PipelineStorageInfo = {
  kind: 'jsonFile' | string;
  description: string;
  path?: string;
};

export function getPipelines() {
  return apiRequest<{ pipelines: PipelineRecord[]; executions: PipelineExecution[]; storage: PipelineStorageInfo }>('/api/pipelines');
}

export function createPipeline(document: PipelineDocument) {
  return apiRequest<PipelineRecord>('/api/pipelines', {
    method: 'POST',
    body: JSON.stringify(document),
  });
}

export function updatePipeline(id: string, document: PipelineDocument) {
  return apiRequest<PipelineRecord>(`/api/pipelines/${id}`, {
    method: 'PUT',
    body: JSON.stringify(document),
  });
}

export function deletePipeline(id: string) {
  return apiRequest<{ deleted: string }>(`/api/pipelines/${id}`, {
    method: 'DELETE',
  });
}

export function executePipeline(document: PipelineDocument) {
  return apiRequest<{ execution: PipelineExecution }>('/api/pipelines/execute', {
    method: 'POST',
    body: JSON.stringify({ document }),
  });
}
