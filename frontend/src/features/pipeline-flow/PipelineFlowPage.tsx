import AddIcon from '@mui/icons-material/Add';
import AccountTreeIcon from '@mui/icons-material/AccountTree';
import DeleteIcon from '@mui/icons-material/Delete';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import SaveIcon from '@mui/icons-material/Save';
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
  useEdgesState,
  useNodesState,
  type Connection,
  type Edge,
  type Node,
  type NodeChange,
  type NodeProps,
} from '@xyflow/react';
import '@xyflow/react/dist/style.css';
import { useCallback, useMemo, useState, type ChangeEvent } from 'react';
import { absoluteImageUrl, getImageResults, type ImageOperation } from '../../api/imageApi';
import {
  createPipeline,
  executePipeline,
  getPipelines,
  updatePipeline,
  type PipelineDocument,
  type PipelineExecution,
  type PipelineFlowNode,
  type PipelineNodeData,
  type PipelineNodeType,
} from '../../api/pipelineApi';

const pipelinesQueryKey = ['pipelines'];
const imageResultsQueryKey = ['image-results'];

type PipelineFlowDisplayData = PipelineNodeData & {
  pipelineType: PipelineNodeType;
};

type PipelineFlowDisplayNode = Node<PipelineFlowDisplayData, 'pipelineNode'>;

const operationOptions: Array<{ value: ImageOperation; label: string; defaultParams?: Record<string, unknown> }> = [
  { value: 'grayscale', label: 'Grayscale' },
  { value: 'blur', label: 'Blur', defaultParams: { kernel: 7 } },
  { value: 'gaussianBlur', label: 'Gaussian Blur', defaultParams: { kernel: 7 } },
  { value: 'threshold', label: 'Threshold', defaultParams: { threshold: 128 } },
  { value: 'edgeDetect', label: 'Edge Detect', defaultParams: { low: 80, high: 160 } },
  { value: 'contourDetect', label: 'Contour Detect', defaultParams: { low: 80, high: 160 } },
  { value: 'histogram', label: 'Histogram' },
  { value: 'sharpen', label: 'Sharpen', defaultParams: { strength: 1 } },
];

function defaultDocument(resultId = ''): PipelineDocument {
  return {
    version: 1,
    name: 'Image Pipeline',
    nodes: [
      {
        id: 'input-1',
        type: 'imageInput',
        position: { x: 0, y: 120 },
        data: { label: 'Image Input', resultId },
      },
      {
        id: 'operation-1',
        type: 'operation',
        position: { x: 260, y: 120 },
        data: { label: 'Grayscale', operation: 'grayscale', params: {} },
      },
      {
        id: 'output-1',
        type: 'output',
        position: { x: 520, y: 120 },
        data: { label: 'Preview Output' },
      },
    ],
    edges: [
      { id: 'input-1-operation-1', source: 'input-1', target: 'operation-1' },
      { id: 'operation-1-output-1', source: 'operation-1', target: 'output-1' },
    ],
  };
}

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
    })),
  };
}

function mutationErrorMessage(error: unknown) {
  return error instanceof Error ? error.message : 'Pipeline operation failed.';
}

function PipelineNodeCard({ data }: NodeProps<PipelineFlowDisplayNode>) {
  const nodeType = data.pipelineType;
  const tone = nodeType === 'imageInput' ? 'primary' : nodeType === 'output' ? 'success' : 'secondary';
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
      }}
    >
      {nodeType !== 'imageInput' && <Handle type="target" position={Position.Left} />}
      <Stack spacing={0.75}>
        <Chip label={nodeType} size="small" color={tone} variant="outlined" sx={{ alignSelf: 'flex-start' }} />
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
      </Stack>
      {nodeType !== 'output' && <Handle type="source" position={Position.Right} />}
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
  const [nodes, setNodes, onNodesChange] = useNodesState<PipelineFlowNode>(defaultDocument().nodes);
  const [edges, setEdges, onEdgesChange] = useEdgesState(defaultDocument().edges);

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

  const selectedNode = nodes.find((node) => node.id === selectedNodeId) ?? nodes[0];
  const document = useMemo(() => toDocument(pipelineName, nodes, edges), [pipelineName, nodes, edges]);
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
  const busy = pipelineQuery.isFetching || imageResultsQuery.isFetching;

  const saveMutation = useMutation({
    mutationFn: () => (pipelineId ? updatePipeline(pipelineId, document) : createPipeline(document)),
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

  const currentError = saveMutation.error ?? executeMutation.error;

  const handleNodesChange = useCallback(
    (changes: NodeChange<PipelineFlowDisplayNode>[]) => {
      onNodesChange(changes as NodeChange<PipelineFlowNode>[]);
    },
    [onNodesChange],
  );

  const onConnect = (connection: Connection) => {
    setEdges((currentEdges) => addEdge({ ...connection, id: `${connection.source}-${connection.target}` }, currentEdges));
  };

  const updateNodeData = (nodeId: string, data: Partial<PipelineNodeData>) => {
    setNodes((currentNodes) =>
      currentNodes.map((node) => (node.id === nodeId ? { ...node, data: { ...node.data, ...data } } : node)),
    );
  };

  const loadPipeline = (recordId: string) => {
    const record = pipelineQuery.data?.pipelines.find((pipeline) => pipeline.id === recordId);
    if (!record) {
      return;
    }
    setPipelineId(record.id);
    setPipelineName(record.document.name);
    setNodes(record.document.nodes);
    setEdges(record.document.edges);
    setSelectedNodeId(record.document.nodes[0]?.id ?? '');
  };

  const resetPipeline = () => {
    const next = defaultDocument(imageResultsQuery.data?.results[0]?.resultId ?? '');
    setPipelineId(null);
    setPipelineName(next.name);
    setNodes(next.nodes);
    setEdges(next.edges);
    setSelectedNodeId('input-1');
    setLastExecution(null);
  };

  const addOperationNode = () => {
    const operationCount = nodes.filter((node) => node.type === 'operation').length + 1;
    const selectedOperation = operationOptions[operationCount % operationOptions.length] ?? operationOptions[0];
    const nodeId = `operation-${Date.now()}`;
    const output = nodes.find((node) => node.type === 'output');
    const beforeOutput = edges.find((edge) => output && edge.target === output.id);
    const source = beforeOutput?.source ?? 'input-1';
    const nextNode: PipelineFlowNode = {
      id: nodeId,
      type: 'operation',
      position: { x: 260 * operationCount, y: 120 },
      data: {
        label: selectedOperation.label,
        operation: selectedOperation.value,
        params: selectedOperation.defaultParams ?? {},
      },
    };

    setNodes((currentNodes) => [...currentNodes, nextNode]);
    setEdges((currentEdges) => [
      ...currentEdges.filter((edge) => edge.id !== beforeOutput?.id),
      { id: `${source}-${nodeId}`, source, target: nodeId },
      ...(output ? [{ id: `${nodeId}-${output.id}`, source: nodeId, target: output.id }] : []),
    ]);
    setSelectedNodeId(nodeId);
  };

  const removeSelectedOperation = () => {
    if (!selectedNode || selectedNode.type !== 'operation') {
      return;
    }

    const incoming = edges.find((edge) => edge.target === selectedNode.id);
    const outgoing = edges.find((edge) => edge.source === selectedNode.id);
    setNodes((currentNodes) => currentNodes.filter((node) => node.id !== selectedNode.id));
    setEdges((currentEdges) => {
      const remaining = currentEdges.filter((edge) => edge.source !== selectedNode.id && edge.target !== selectedNode.id);
      if (incoming && outgoing) {
        return [...remaining, { id: `${incoming.source}-${outgoing.target}`, source: incoming.source, target: outgoing.target }];
      }
      return remaining;
    });
    setSelectedNodeId('input-1');
  };

  const changeSelectedOperation = (event: ChangeEvent<HTMLInputElement>) => {
    const nextOperation = event.target.value as ImageOperation;
    const option = operationOptions.find((candidate) => candidate.value === nextOperation);
    if (!selectedNode) {
      return;
    }
    updateNodeData(selectedNode.id, {
      label: option?.label ?? nextOperation,
      operation: nextOperation,
      params: option?.defaultParams ?? {},
    });
  };

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
            <Button variant="outlined" onClick={resetPipeline}>
              New
            </Button>
            <Button startIcon={<SaveIcon />} variant="outlined" onClick={() => saveMutation.mutate()} disabled={saveMutation.isPending}>
              Save
            </Button>
            <Button startIcon={<PlayArrowIcon />} variant="contained" onClick={() => executeMutation.mutate()} disabled={executeMutation.isPending}>
              Run
            </Button>
          </Stack>
        </Stack>

        {isMobile && (
          <Alert severity="info">
            Mobile clients can inspect and run saved pipelines. Complex node editing is limited to desktop and tablet layouts.
          </Alert>
        )}
        {currentError && <Alert severity="error">{mutationErrorMessage(currentError)}</Alert>}
        {execution?.status === 'failed' && <Alert severity="error">{execution.error ?? 'Pipeline execution failed.'}</Alert>}

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
                    '& .react-flow__node:focus, & .react-flow__node:focus-visible, & .react-flow__node.selected': {
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
                      fill: alpha(theme.palette.background.default, theme.palette.mode === 'dark' ? 0.72 : 0.56),
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
                    nodesDraggable={!isMobile}
                    nodesConnectable={!isMobile}
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
                        maskColor={alpha(theme.palette.background.default, theme.palette.mode === 'dark' ? 0.72 : 0.56)}
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
                    <TextField label="Name" size="small" value={pipelineName} onChange={(event) => setPipelineName(event.target.value)} />
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
                    <Stack direction="row" spacing={1}>
                      <Button startIcon={<AddIcon />} variant="outlined" onClick={addOperationNode} disabled={isMobile}>
                        Node
                      </Button>
                      <Button
                        startIcon={<DeleteIcon />}
                        color="error"
                        variant="outlined"
                        onClick={removeSelectedOperation}
                        disabled={isMobile || selectedNode?.type !== 'operation'}
                      >
                        Remove
                      </Button>
                    </Stack>
                    <Divider />
                    <Stack direction={{ xs: 'column', sm: 'row' }} spacing={1} flexWrap="wrap">
                      <FormControlLabel
                        control={<Switch size="small" checked={showGrid} onChange={(event) => setShowGrid(event.target.checked)} />}
                        label="Grid"
                      />
                      <FormControlLabel
                        control={<Switch size="small" checked={showControls} onChange={(event) => setShowControls(event.target.checked)} />}
                        label="Controls"
                      />
                      <FormControlLabel
                        control={<Switch size="small" checked={showMiniMap} onChange={(event) => setShowMiniMap(event.target.checked)} />}
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
                        onChange={(event) => updateNodeData(selectedNode.id, { resultId: event.target.value })}
                      >
                        <MenuItem value="">Select a result</MenuItem>
                        {(imageResultsQuery.data?.results ?? []).map((result) => (
                          <MenuItem key={result.resultId} value={result.resultId}>
                            {result.name} · {result.operation}
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
                      <Typography color="text.secondary">The output node publishes the final preview result.</Typography>
                    )}
                    <Typography variant="caption" color="text.secondary">
                      {busy ? 'Refreshing server-owned pipeline state...' : `Selected: ${selectedNode?.id ?? 'none'}`}
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
                          <Chip label={execution.status} color={execution.status === 'completed' ? 'success' : 'error'} size="small" />
                          <Chip label={`job ${execution.job.id}`} size="small" variant="outlined" />
                        </Stack>
                        <Divider />
                        <Stack spacing={1}>
                          {execution.steps.map((step) => (
                            <Stack key={`${execution.executionId}-${step.nodeId}`} direction="row" spacing={1} alignItems="center">
                              <Chip label={step.status} size="small" color="success" variant="outlined" />
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
                      <Typography color="text.secondary">Run the pipeline to see synchronized node execution state.</Typography>
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
