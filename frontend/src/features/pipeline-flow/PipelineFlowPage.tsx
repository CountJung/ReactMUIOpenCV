import AddIcon from '@mui/icons-material/Add';
import AccountTreeIcon from '@mui/icons-material/AccountTree';
import DeleteIcon from '@mui/icons-material/Delete';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import SaveIcon from '@mui/icons-material/Save';
import UndoIcon from '@mui/icons-material/Undo';
import {
  Alert,
  Box,
  Button,
  Card,
  CardContent,
  Chip,
  Divider,
  FormControlLabel,
  Grid,
  MenuItem,
  Stack,
  Switch,
  TextField,
  Typography,
  useMediaQuery,
  useTheme,
} from '@mui/material';
import { alpha } from '@mui/material/styles';
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import {
  Background,
  Controls,
  Handle,
  MiniMap,
  Position,
  ReactFlow,
  ReactFlowProvider,
  addEdge,
  ConnectionMode,
  useEdgesState,
  useNodesState,
  type Connection,
  type Edge,
  type Node,
  type NodeChange,
  type NodeProps,
} from '@xyflow/react';
import '@xyflow/react/dist/style.css';
import { useCallback, useEffect, useMemo, useRef, useState, type ChangeEvent } from 'react';
import { absoluteImageUrl, getImageResults } from '../../api/imageApi';
import {
  createPipeline,
  deletePipeline,
  executePipeline,
  getPipelines,
  updatePipeline,
  type PipelineDocument,
  type PipelineExecution,
  type PipelineFlowNode,
  type PipelineNodeData,
  type PipelineNodeType,
  type PipelineOperation,
} from '../../api/pipelineApi';
import { getVideos } from '../../api/videoApi';
import { defaultDocument, nodeKindOptions, operationOptions } from './pipelineOptions';

const pipelinesQueryKey = ['pipelines'];
const imageResultsQueryKey = ['image-results'];

type PipelineFlowDisplayData = PipelineNodeData & {
  pipelineType: PipelineNodeType;
};

type PipelineFlowDisplayNode = Node<PipelineFlowDisplayData, 'pipelineNode'>;

type PipelineSnapshot = {
  pipelineId: string | null;
  pipelineName: string;
  selectedNodeId: string;
  nodes: PipelineFlowNode[];
  edges: Edge[];
};

function toDocument(name: string, nodes: PipelineFlowNode[], edges: Edge[]): PipelineDocument {
  return {
    version: 1,
    name,
    nodes: nodes.map((node) => ({
      id: node.id,
      type: node.type ?? 'operation',
      position: node.position,
      data: node.data,
    })),
    edges: edges.map((edge) => ({
      id: edge.id,
      source: edge.source,
      target: edge.target,
      sourceHandle: edge.sourceHandle,
      targetHandle: edge.targetHandle,
    })),
  };
}

function mutationErrorMessage(error: unknown) {
  return error instanceof Error ? error.message : 'Pipeline operation failed.';
}

function isEditableTarget(target: EventTarget | null) {
  if (!(target instanceof HTMLElement)) {
    return false;
  }

  return Boolean(target.closest('input, textarea, [contenteditable="true"]'));
}

function cloneNodes(nodes: PipelineFlowNode[]) {
  return nodes.map((node) => ({
    ...node,
    position: { ...node.position },
    data: { ...node.data, params: node.data.params ? { ...node.data.params } : undefined },
  }));
}

function cloneEdges(edges: Edge[]) {
  return edges.map((edge) => ({ ...edge }));
}

function sameIds(left: string[], right: string[]) {
  return left.length === right.length && left.every((id, index) => id === right[index]);
}

const handlePoints: Array<{
  id: string;
  position: Position;
  style: Record<string, number | string>;
}> = [
  { id: 'top-left', position: Position.Top, style: { left: 24 } },
  { id: 'top', position: Position.Top, style: { left: '50%' } },
  { id: 'top-right', position: Position.Top, style: { left: 'calc(100% - 24px)' } },
  { id: 'right', position: Position.Right, style: { top: '50%' } },
  { id: 'bottom-right', position: Position.Bottom, style: { left: 'calc(100% - 24px)' } },
  { id: 'bottom', position: Position.Bottom, style: { left: '50%' } },
  { id: 'bottom-left', position: Position.Bottom, style: { left: 24 } },
  { id: 'left', position: Position.Left, style: { top: '50%' } },
];

function ConnectionHandles({ isConnectable }: { isConnectable: boolean }) {
  return (
    <>
      {handlePoints.map((point) => (
        <Handle
          key={`${point.id}-source`}
          id={`${point.id}-source`}
          type="source"
          position={point.position}
          isConnectable={isConnectable}
          style={point.style}
        />
      ))}
    </>
  );
}

function PipelineNodeCard({ data, isConnectable }: NodeProps<PipelineFlowDisplayNode>) {
  const nodeType = data.pipelineType;
  const tone =
    nodeType === 'imageInput' ? 'primary' : nodeType === 'output' ? 'success' : 'secondary';
  return (
    <Box
      className="pipeline-node-card"
      sx={{
        bgcolor: 'background.paper',
        border: 1,
        borderColor: 'divider',
        borderRadius: 2,
        boxShadow: 1,
        minWidth: 172,
        p: 1.25,
        position: 'relative',
      }}
    >
      <ConnectionHandles isConnectable={isConnectable} />
      <Stack spacing={0.75}>
        <Chip
          label={nodeType}
          size="small"
          color={tone}
          variant="outlined"
          sx={{ alignSelf: 'flex-start' }}
        />
        <Typography variant="subtitle2">{data.label}</Typography>
        {nodeType === 'operation' && (
          <Typography variant="caption" color="text.secondary">
            {data.operation ?? 'operation'}
          </Typography>
        )}
        {nodeType === 'imageInput' && (
          <Typography variant="caption" color="text.secondary" noWrap>
            {data.resultId ? `result: ${data.resultId}` : 'select image result'}
          </Typography>
        )}
        {nodeType === 'videoInput' && (
          <Typography variant="caption" color="text.secondary" noWrap>
            {data.videoId ? `video: ${data.videoId}` : 'select video'}
          </Typography>
        )}
      </Stack>
    </Box>
  );
}

const nodeTypes = {
  pipelineNode: PipelineNodeCard,
};

export function PipelineFlowPage() {
  return (
    <ReactFlowProvider>
      <PipelineFlowWorkspace />
    </ReactFlowProvider>
  );
}

function PipelineFlowWorkspace() {
  const queryClient = useQueryClient();
  const theme = useTheme();
  const isMobile = useMediaQuery(theme.breakpoints.down('sm'));
  const [pipelineId, setPipelineId] = useState<string | null>(null);
  const [pipelineName, setPipelineName] = useState('Image Pipeline');
  const [selectedNodeId, setSelectedNodeId] = useState<string>('input-1');
  const [lastExecution, setLastExecution] = useState<PipelineExecution | null>(null);
  const [showGrid, setShowGrid] = useState(true);
  const [showControls, setShowControls] = useState(true);
  const [showMiniMap, setShowMiniMap] = useState(false);
  const [newNodeKind, setNewNodeKind] = useState<PipelineNodeType>('operation');
  const [history, setHistory] = useState<PipelineSnapshot[]>([]);
  const [selectedGraph, setSelectedGraph] = useState<{ nodeIds: string[]; edgeIds: string[] }>({
    nodeIds: ['input-1'],
    edgeIds: [],
  });
  const [nodes, setNodes, onNodesChange] = useNodesState<PipelineFlowNode>(defaultDocument().nodes);
  const [edges, setEdges, onEdgesChange] = useEdgesState(defaultDocument().edges);
  const graphRef = useRef({ nodes, edges, pipelineId, pipelineName, selectedNodeId });

  useEffect(() => {
    graphRef.current = { nodes, edges, pipelineId, pipelineName, selectedNodeId };
  }, [edges, nodes, pipelineId, pipelineName, selectedNodeId]);

  const pipelineQuery = useQuery({
    queryKey: pipelinesQueryKey,
    queryFn: getPipelines,
    refetchInterval: 8000,
  });

  const imageResultsQuery = useQuery({
    queryKey: imageResultsQueryKey,
    queryFn: getImageResults,
    refetchInterval: 8000,
  });
  const videosQuery = useQuery({
    queryKey: ['video-library'],
    queryFn: getVideos,
    refetchInterval: 8000,
  });

  const selectedNode = nodes.find((node) => node.id === selectedNodeId) ?? nodes[0];
  const document = useMemo(
    () => toDocument(pipelineName, nodes, edges),
    [pipelineName, nodes, edges],
  );
  const flowNodes = useMemo<PipelineFlowDisplayNode[]>(
    () =>
      nodes.map((node) => ({
        ...node,
        type: 'pipelineNode',
        data: {
          ...node.data,
          pipelineType: node.type ?? 'operation',
        },
      })),
    [nodes],
  );
  const execution = lastExecution ?? pipelineQuery.data?.executions?.[0] ?? null;
  const busy = pipelineQuery.isFetching || imageResultsQuery.isFetching || videosQuery.isFetching;

  const saveMutation = useMutation({
    mutationFn: () =>
      pipelineId ? updatePipeline(pipelineId, document) : createPipeline(document),
    onSuccess: (pipeline) => {
      setPipelineId(pipeline.id);
      setPipelineName(pipeline.document.name);
      void queryClient.invalidateQueries({ queryKey: pipelinesQueryKey });
    },
  });

  const executeMutation = useMutation({
    mutationFn: () => executePipeline(document),
    onSuccess: ({ execution }) => {
      setLastExecution(execution);
      void queryClient.invalidateQueries({ queryKey: pipelinesQueryKey });
      void queryClient.invalidateQueries({ queryKey: imageResultsQueryKey });
      void queryClient.invalidateQueries({ queryKey: ['jobs'] });
    },
  });

  const deleteSavedMutation = useMutation({
    mutationFn: (id: string) => deletePipeline(id),
    onSuccess: (_, deletedId) => {
      if (pipelineId === deletedId) {
        resetPipeline();
      }
      void queryClient.invalidateQueries({ queryKey: pipelinesQueryKey });
    },
  });

  const currentError = saveMutation.error ?? executeMutation.error ?? deleteSavedMutation.error;

  const pushHistory = useCallback((label: string) => {
    const snapshot = graphRef.current;
    setHistory((items) =>
      [
        ...items,
        {
          pipelineId: snapshot.pipelineId,
          pipelineName: snapshot.pipelineName,
          selectedNodeId: snapshot.selectedNodeId,
          nodes: cloneNodes(snapshot.nodes),
          edges: cloneEdges(snapshot.edges),
        },
      ].slice(-30),
    );
    return label;
  }, []);

  const undoLastChange = useCallback(() => {
    setHistory((items) => {
      const last = items.at(-1);
      if (!last) {
        return items;
      }

      setPipelineId(last.pipelineId);
      setPipelineName(last.pipelineName);
      setSelectedNodeId(last.selectedNodeId);
      setSelectedGraph({ nodeIds: last.selectedNodeId ? [last.selectedNodeId] : [], edgeIds: [] });
      setNodes(cloneNodes(last.nodes));
      setEdges(cloneEdges(last.edges));
      return items.slice(0, -1);
    });
  }, [setEdges, setNodes]);

  const handleNodesChange = useCallback(
    (changes: NodeChange<PipelineFlowDisplayNode>[]) => {
      onNodesChange(changes as NodeChange<PipelineFlowNode>[]);
    },
    [onNodesChange],
  );

  const onConnect = useCallback(
    (connection: Connection) => {
      pushHistory('connect');
      const id = [
        connection.source,
        connection.sourceHandle ?? 'source',
        connection.target,
        connection.targetHandle ?? 'target',
      ].join('-');
      setEdges((currentEdges) => addEdge({ ...connection, id }, currentEdges));
    },
    [pushHistory, setEdges],
  );

  const handleSelectionChange = useCallback(
    ({
      nodes: selectedNodes,
      edges: selectedEdges,
    }: {
      nodes: PipelineFlowDisplayNode[];
      edges: Edge[];
    }) => {
      const nextNodeIds = selectedNodes.map((node) => node.id);
      const nextEdgeIds = selectedEdges.map((edge) => edge.id);
      setSelectedGraph((current) =>
        sameIds(current.nodeIds, nextNodeIds) && sameIds(current.edgeIds, nextEdgeIds)
          ? current
          : { nodeIds: nextNodeIds, edgeIds: nextEdgeIds },
      );

      const nextSelectedNodeId = selectedNodes[0]?.id;
      if (nextSelectedNodeId) {
        setSelectedNodeId((current) =>
          current === nextSelectedNodeId ? current : nextSelectedNodeId,
        );
      }
    },
    [],
  );

  const updateNodeData = (nodeId: string, data: Partial<PipelineNodeData>) => {
    setNodes((currentNodes) =>
      currentNodes.map((node) =>
        node.id === nodeId ? { ...node, data: { ...node.data, ...data } } : node,
      ),
    );
  };

  const loadPipeline = (recordId: string) => {
    const record = pipelineQuery.data?.pipelines.find((pipeline) => pipeline.id === recordId);
    if (!record) {
      return;
    }
    pushHistory('load');
    setPipelineId(record.id);
    setPipelineName(record.document.name);
    setNodes(record.document.nodes);
    setEdges(record.document.edges);
    const nextSelectedId = record.document.nodes[0]?.id ?? '';
    setSelectedNodeId(nextSelectedId);
    setSelectedGraph({ nodeIds: nextSelectedId ? [nextSelectedId] : [], edgeIds: [] });
  };

  const resetPipeline = () => {
    pushHistory('reset');
    const next = defaultDocument(imageResultsQuery.data?.results[0]?.resultId ?? '');
    setPipelineId(null);
    setPipelineName(next.name);
    setNodes(next.nodes);
    setEdges(next.edges);
    setSelectedNodeId('input-1');
    setSelectedGraph({ nodeIds: ['input-1'], edgeIds: [] });
    setLastExecution(null);
  };

  const addPipelineNode = () => {
    pushHistory('add-node');
    const operationCount = nodes.filter((node) => node.type === 'operation').length + 1;
    const selectedOperation =
      operationOptions[operationCount % operationOptions.length] ?? operationOptions[0];
    const nodeId = `${newNodeKind}-${Date.now()}`;
    const output = nodes.find((node) => node.type === 'output');
    const beforeOutput = edges.find((edge) => output && edge.target === output.id);
    const source = beforeOutput?.source ?? 'input-1';
    const selectedNode = nodes.find((node) => node.id === selectedNodeId);
    const basePosition = selectedNode?.position ?? { x: 0, y: 120 };
    const nextNode: PipelineFlowNode = {
      id: nodeId,
      type: newNodeKind,
      position: {
        x: basePosition.x + 220,
        y: basePosition.y + (newNodeKind === 'operation' ? 0 : 120),
      },
      data:
        newNodeKind === 'operation'
          ? {
              label: selectedOperation.label,
              operation: selectedOperation.value,
              params: selectedOperation.defaultParams ?? {},
            }
          : newNodeKind === 'imageInput'
            ? {
                label: `Image Input ${nodes.filter((node) => node.type === 'imageInput').length + 1}`,
                resultId: imageResultsQuery.data?.results[0]?.resultId ?? '',
              }
            : newNodeKind === 'videoInput'
              ? {
                  label: `Video Input ${nodes.filter((node) => node.type === 'videoInput').length + 1}`,
                  videoId: videosQuery.data?.videos[0]?.videoId ?? '',
                }
              : {
                  label: `Output ${nodes.filter((node) => node.type === 'output').length + 1}`,
                },
    };

    setNodes((currentNodes) => [...currentNodes, nextNode]);
    if (newNodeKind === 'operation') {
      setEdges((currentEdges) => [
        ...currentEdges.filter((edge) => edge.id !== beforeOutput?.id),
        {
          id: `${source}-${nodeId}`,
          source,
          target: nodeId,
          sourceHandle: 'right-source',
          targetHandle: 'left-source',
        },
        ...(output
          ? [
              {
                id: `${nodeId}-${output.id}`,
                source: nodeId,
                target: output.id,
                sourceHandle: 'right-source',
                targetHandle: 'left-source',
              },
            ]
          : []),
      ]);
    }
    setSelectedNodeId(nodeId);
    setSelectedGraph({ nodeIds: [nodeId], edgeIds: [] });
  };

  const deleteSelectedElements = useCallback(() => {
    const snapshot = graphRef.current;
    const selectedNodeIds = new Set(selectedGraph.nodeIds);
    const selectedEdgeIds = new Set(selectedGraph.edgeIds);
    if (selectedNodeIds.size === 0 && selectedEdgeIds.size === 0) {
      return;
    }

    pushHistory('delete-selection');
    setNodes((currentNodes) => currentNodes.filter((node) => !selectedNodeIds.has(node.id)));
    setEdges((currentEdges) =>
      currentEdges.filter(
        (edge) =>
          !selectedEdgeIds.has(edge.id) &&
          !selectedNodeIds.has(edge.source) &&
          !selectedNodeIds.has(edge.target),
      ),
    );
    const nextNode = snapshot.nodes.find((node) => !selectedNodeIds.has(node.id));
    setSelectedNodeId(nextNode?.id ?? '');
    setSelectedGraph({ nodeIds: nextNode ? [nextNode.id] : [], edgeIds: [] });
  }, [pushHistory, selectedGraph.edgeIds, selectedGraph.nodeIds, setEdges, setNodes]);

  const changeSelectedOperation = (event: ChangeEvent<HTMLInputElement>) => {
    const nextOperation = event.target.value as PipelineOperation;
    const option = operationOptions.find((candidate) => candidate.value === nextOperation);
    if (!selectedNode) {
      return;
    }
    pushHistory('change-operation');
    updateNodeData(selectedNode.id, {
      label: option?.label ?? nextOperation,
      operation: nextOperation,
      params: option?.defaultParams ?? {},
    });
  };

  useEffect(() => {
    if (isMobile) {
      return undefined;
    }

    const handleKeyDown = (event: KeyboardEvent) => {
      if (isEditableTarget(event.target)) {
        return;
      }

      if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'z') {
        event.preventDefault();
        undoLastChange();
        return;
      }

      if (event.key === 'Delete' || event.key === 'Backspace') {
        event.preventDefault();
        deleteSelectedElements();
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [deleteSelectedElements, isMobile, undoLastChange]);

  return (
    <Box sx={{ p: { xs: 2, md: 3 } }}>
      <Stack spacing={2.5}>
        <Stack direction={{ xs: 'column', md: 'row' }} justifyContent="space-between" spacing={2}>
          <Stack spacing={0.75}>
            <Stack direction="row" spacing={1} alignItems="center" flexWrap="wrap">
              <AccountTreeIcon color="primary" fontSize="small" />
              <Typography variant="overline" color="text.secondary">
                React Flow
              </Typography>
              <Chip label="Phase 6" size="small" color="primary" variant="outlined" />
            </Stack>
            <Typography variant="h4" component="h1">
              Pipeline Flow
            </Typography>
          </Stack>

          <Stack direction="row" spacing={1} flexWrap="wrap">
            <Button
              startIcon={<UndoIcon />}
              variant="outlined"
              onClick={undoLastChange}
              disabled={history.length === 0}
            >
              Undo
            </Button>
            <Button variant="outlined" onClick={resetPipeline}>
              New
            </Button>
            <Button
              startIcon={<SaveIcon />}
              variant="outlined"
              onClick={() => saveMutation.mutate()}
              disabled={saveMutation.isPending}
            >
              Save
            </Button>
            <Button
              startIcon={<DeleteIcon />}
              color="error"
              variant="outlined"
              onClick={() => pipelineId && deleteSavedMutation.mutate(pipelineId)}
              disabled={!pipelineId || deleteSavedMutation.isPending}
            >
              Delete
            </Button>
            <Button
              startIcon={<PlayArrowIcon />}
              variant="contained"
              onClick={() => executeMutation.mutate()}
              disabled={executeMutation.isPending}
            >
              Run
            </Button>
          </Stack>
        </Stack>

        {isMobile && (
          <Alert severity="info">
            Mobile clients can inspect and run saved pipelines. Complex node editing is limited to
            desktop and tablet layouts.
          </Alert>
        )}
        {currentError && <Alert severity="error">{mutationErrorMessage(currentError)}</Alert>}
        {execution?.status === 'failed' && (
          <Alert severity="error">{execution.error ?? 'Pipeline execution failed.'}</Alert>
        )}

        <Grid container spacing={2}>
          <Grid item xs={12} lg={8}>
            <Card sx={{ overflow: 'hidden' }}>
              <CardContent sx={{ p: 0 }}>
                <Box
                  sx={{
                    bgcolor: 'background.default',
                    height: { xs: 420, md: 620 },
                    '& .react-flow': {
                      bgcolor: 'background.default',
                      color: 'text.primary',
                    },
                    '& .react-flow__node': {
                      background: 'transparent',
                      border: 0,
                      boxShadow: 'none !important',
                      p: 0,
                    },
                    '& .react-flow__node:focus, & .react-flow__node:focus-visible, & .react-flow__node.selected':
                      {
                        outline: 'none',
                        boxShadow: 'none !important',
                      },
                    '& .react-flow__node.selected .pipeline-node-card': {
                      borderColor: 'primary.main',
                      boxShadow: `0 0 0 3px ${alpha(theme.palette.primary.main, 0.24)}`,
                    },
                    '& .react-flow__edge-path': {
                      stroke: 'divider',
                    },
                    '& .react-flow__edge.selected .react-flow__edge-path': {
                      stroke: 'primary.main',
                    },
                    '& .react-flow__handle': {
                      bgcolor: 'background.paper',
                      borderColor: 'text.secondary',
                      height: 9,
                      width: 9,
                    },
                    '& .react-flow__handle-source': {
                      bgcolor: 'primary.main',
                      borderColor: 'background.paper',
                    },
                    '& .react-flow__handle-connecting, & .react-flow__handle-valid': {
                      bgcolor: 'success.main',
                    },
                    '& .react-flow__controls': {
                      bgcolor: 'background.paper !important',
                      border: 1,
                      borderColor: 'divider',
                      borderRadius: 1,
                      boxShadow: 2,
                      overflow: 'hidden',
                    },
                    '& .react-flow__controls-button': {
                      bgcolor: 'background.paper !important',
                      borderBottom: `1px solid ${theme.palette.divider} !important`,
                      color: 'text.primary !important',
                      height: 32,
                      width: 32,
                    },
                    '& .react-flow__controls-button:hover': {
                      bgcolor: 'action.hover !important',
                    },
                    '& .react-flow__controls-button svg': {
                      fill: 'currentColor !important',
                    },
                    '& .react-flow__minimap': {
                      bgcolor: 'background.paper !important',
                      border: 1,
                      borderColor: 'divider',
                      borderRadius: 1,
                      boxShadow: 2,
                    },
                    '& .react-flow__minimap-mask': {
                      fill: alpha(
                        theme.palette.background.default,
                        theme.palette.mode === 'dark' ? 0.72 : 0.56,
                      ),
                    },
                    '& .react-flow__minimap-node': {
                      stroke: 'divider',
                    },
                  }}
                >
                  <ReactFlow
                    nodes={flowNodes}
                    edges={edges}
                    nodeTypes={nodeTypes}
                    onNodesChange={isMobile ? undefined : handleNodesChange}
                    onEdgesChange={isMobile ? undefined : onEdgesChange}
                    onConnect={isMobile ? undefined : onConnect}
                    onNodeClick={(_, node) => setSelectedNodeId(node.id)}
                    onSelectionChange={handleSelectionChange}
                    nodesDraggable={!isMobile}
                    nodesConnectable={!isMobile}
                    connectionMode={ConnectionMode.Loose}
                    deleteKeyCode={null}
                    colorMode={theme.palette.mode}
                    fitView
                    fitViewOptions={{ padding: 0.25 }}
                    minZoom={0.45}
                    maxZoom={1.8}
                    proOptions={{ hideAttribution: true }}
                  >
                    {showGrid && <Background color={theme.palette.divider} gap={18} size={1} />}
                    {showControls && <Controls showInteractive={false} />}
                    {showMiniMap && (
                      <MiniMap
                        pannable
                        zoomable
                        maskColor={alpha(
                          theme.palette.background.default,
                          theme.palette.mode === 'dark' ? 0.72 : 0.56,
                        )}
                        nodeColor={(node) => {
                          const pipelineType = node.data.pipelineType;
                          if (pipelineType === 'imageInput') {
                            return theme.palette.primary.main;
                          }
                          if (pipelineType === 'output') {
                            return theme.palette.success.main;
                          }
                          return theme.palette.secondary.main;
                        }}
                        style={{
                          backgroundColor: theme.palette.background.paper,
                          border: `1px solid ${theme.palette.divider}`,
                          borderRadius: theme.shape.borderRadius,
                          boxShadow: theme.shadows[2],
                        }}
                      />
                    )}
                  </ReactFlow>
                </Box>
              </CardContent>
            </Card>
          </Grid>

          <Grid item xs={12} lg={4}>
            <Stack spacing={2}>
              <Card>
                <CardContent>
                  <Stack spacing={2}>
                    <Typography variant="h6">Pipeline</Typography>
                    <TextField
                      label="Name"
                      size="small"
                      value={pipelineName}
                      onChange={(event) => setPipelineName(event.target.value)}
                    />
                    <TextField
                      label="Saved Pipeline"
                      select
                      size="small"
                      value={pipelineId ?? ''}
                      onChange={(event) => loadPipeline(event.target.value)}
                    >
                      <MenuItem value="">Unsaved pipeline</MenuItem>
                      {(pipelineQuery.data?.pipelines ?? []).map((pipeline) => (
                        <MenuItem key={pipeline.id} value={pipeline.id}>
                          {pipeline.document.name}
                        </MenuItem>
                      ))}
                    </TextField>
                    <Stack spacing={0.75}>
                      <Typography variant="caption" color="text.secondary">
                        Storage
                      </Typography>
                      <Typography
                        variant="body2"
                        sx={{
                          bgcolor: 'background.default',
                          border: 1,
                          borderColor: 'divider',
                          borderRadius: 1,
                          fontFamily: 'monospace',
                          overflowWrap: 'anywhere',
                          p: 1,
                        }}
                      >
                        {pipelineQuery.data?.storage.path ??
                          pipelineQuery.data?.storage.description ??
                          'Backend pipeline storage'}
                      </Typography>
                    </Stack>
                    <TextField
                      label="New Node Type"
                      select
                      size="small"
                      value={newNodeKind}
                      onChange={(event) => setNewNodeKind(event.target.value as PipelineNodeType)}
                      disabled={isMobile}
                    >
                      {nodeKindOptions.map((option) => (
                        <MenuItem key={option.value} value={option.value}>
                          {option.label}
                        </MenuItem>
                      ))}
                    </TextField>
                    <Stack direction="row" spacing={1}>
                      <Button
                        startIcon={<AddIcon />}
                        variant="outlined"
                        onClick={addPipelineNode}
                        disabled={isMobile}
                      >
                        Node
                      </Button>
                      <Button
                        startIcon={<DeleteIcon />}
                        color="error"
                        variant="outlined"
                        onClick={deleteSelectedElements}
                        disabled={
                          isMobile ||
                          (selectedGraph.nodeIds.length === 0 && selectedGraph.edgeIds.length === 0)
                        }
                      >
                        Selection
                      </Button>
                    </Stack>
                    <Typography variant="caption" color="text.secondary">
                      Connect visible dots from any side. Delete or Backspace removes selected nodes
                      or lead lines; Ctrl+Z restores the previous graph state.
                    </Typography>
                    <Divider />
                    <Stack direction={{ xs: 'column', sm: 'row' }} spacing={1} flexWrap="wrap">
                      <FormControlLabel
                        control={
                          <Switch
                            size="small"
                            checked={showGrid}
                            onChange={(event) => setShowGrid(event.target.checked)}
                          />
                        }
                        label="Grid"
                      />
                      <FormControlLabel
                        control={
                          <Switch
                            size="small"
                            checked={showControls}
                            onChange={(event) => setShowControls(event.target.checked)}
                          />
                        }
                        label="Controls"
                      />
                      <FormControlLabel
                        control={
                          <Switch
                            size="small"
                            checked={showMiniMap}
                            onChange={(event) => setShowMiniMap(event.target.checked)}
                          />
                        }
                        label="Mini map"
                      />
                    </Stack>
                  </Stack>
                </CardContent>
              </Card>

              <Card>
                <CardContent>
                  <Stack spacing={2}>
                    <Typography variant="h6">Selected Node</Typography>
                    {selectedNode?.type === 'imageInput' && (
                      <TextField
                        label="Image Result"
                        select
                        size="small"
                        value={String(selectedNode.data.resultId ?? '')}
                        onChange={(event) =>
                          updateNodeData(selectedNode.id, { resultId: event.target.value })
                        }
                      >
                        <MenuItem value="">Select a result</MenuItem>
                        {(imageResultsQuery.data?.results ?? []).map((result) => (
                          <MenuItem key={result.resultId} value={result.resultId}>
                            {result.name} · {result.operation}
                          </MenuItem>
                        ))}
                      </TextField>
                    )}
                    {selectedNode?.type === 'videoInput' && (
                      <TextField
                        label="Video"
                        select
                        size="small"
                        value={String(selectedNode.data.videoId ?? '')}
                        onChange={(event) =>
                          updateNodeData(selectedNode.id, { videoId: event.target.value })
                        }
                      >
                        <MenuItem value="">Select a video</MenuItem>
                        {(videosQuery.data?.videos ?? []).map((video) => (
                          <MenuItem key={video.videoId} value={video.videoId}>
                            {video.name} · {video.width}x{video.height}
                          </MenuItem>
                        ))}
                      </TextField>
                    )}
                    {selectedNode?.type === 'operation' && (
                      <TextField
                        label="Operation"
                        select
                        size="small"
                        value={String(selectedNode.data.operation ?? 'grayscale')}
                        onChange={changeSelectedOperation}
                        disabled={isMobile}
                      >
                        {operationOptions.map((option) => (
                          <MenuItem key={option.value} value={option.value}>
                            {option.label}
                          </MenuItem>
                        ))}
                      </TextField>
                    )}
                    {selectedNode?.type === 'output' && (
                      <Typography color="text.secondary">
                        The output node publishes the final preview result.
                      </Typography>
                    )}
                    <Typography variant="caption" color="text.secondary">
                      {busy
                        ? 'Refreshing server-owned pipeline state...'
                        : `Selected: ${selectedNode?.id ?? 'none'}`}
                    </Typography>
                  </Stack>
                </CardContent>
              </Card>

              <Card>
                <CardContent>
                  <Stack spacing={2}>
                    <Typography variant="h6">Execution</Typography>
                    {execution ? (
                      <>
                        <Stack direction="row" spacing={1} flexWrap="wrap">
                          <Chip
                            label={execution.status}
                            color={execution.status === 'completed' ? 'success' : 'error'}
                            size="small"
                          />
                          <Chip label={`job ${execution.job.id}`} size="small" variant="outlined" />
                        </Stack>
                        <Divider />
                        <Stack spacing={1}>
                          {execution.steps.map((step) => (
                            <Stack
                              key={`${execution.executionId}-${step.nodeId}`}
                              direction="row"
                              spacing={1}
                              alignItems="center"
                            >
                              <Chip
                                label={step.status}
                                size="small"
                                color="success"
                                variant="outlined"
                              />
                              <Typography variant="body2">{step.nodeId}</Typography>
                            </Stack>
                          ))}
                        </Stack>
                        {execution.result?.previewUrl && (
                          <Box
                            component="img"
                            alt="Pipeline result preview"
                            src={absoluteImageUrl(execution.result.previewUrl)}
                            sx={{
                              bgcolor: 'background.default',
                              border: 1,
                              borderColor: 'divider',
                              borderRadius: 2,
                              maxHeight: 260,
                              objectFit: 'contain',
                              width: '100%',
                            }}
                          />
                        )}
                      </>
                    ) : (
                      <Typography color="text.secondary">
                        Run the pipeline to see synchronized node execution state.
                      </Typography>
                    )}
                  </Stack>
                </CardContent>
              </Card>
            </Stack>
          </Grid>
        </Grid>
      </Stack>
    </Box>
  );
}
