import DeleteIcon from '@mui/icons-material/Delete';
import FileOpenIcon from '@mui/icons-material/FileOpen';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import SpeedIcon from '@mui/icons-material/Speed';
import UploadFileIcon from '@mui/icons-material/UploadFile';
import {
  Alert,
  Box,
  Button,
  Card,
  CardContent,
  Chip,
  Grid,
  IconButton,
  LinearProgress,
  MenuItem,
  Stack,
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableRow,
  TextField,
  Tooltip,
  Typography,
} from '@mui/material';
import { alpha } from '@mui/material/styles';
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import { useEffect, useMemo, useState } from 'react';
import {
  clearPerformanceBenchmarks,
  deletePerformanceBenchmark,
  getPerformanceBenchmarks,
  runPerformanceBenchmark,
  type PerformanceBenchmarkMethod,
  type PerformanceBenchmarkRecord,
} from '../../api/performanceApi';
import {
  absoluteImageUrl,
  getImageResults,
  openLocalImage,
  uploadImageFile,
} from '../../api/imageApi';
import { getSettings } from '../../api/settingsApi';
import { PlaceholderPage } from '../../shared/components/PlaceholderPage';

function MethodBars({ methods }: { methods: PerformanceBenchmarkMethod[] }) {
  const max = Math.max(...methods.map((method) => method.megapixelsPerSecond), 1);

  return (
    <Stack spacing={1.25}>
      {methods.map((method) => (
        <Stack key={method.name} spacing={0.5}>
          <Stack direction="row" justifyContent="space-between" spacing={1}>
            <Typography variant="body2">{method.label}</Typography>
            <Typography variant="body2" color="text.secondary">
              {method.megapixelsPerSecond.toFixed(1)} MP/s
            </Typography>
          </Stack>
          <Box
            sx={(theme) => ({
              bgcolor: alpha(theme.palette.primary.main, 0.12),
              borderRadius: 1,
              height: 10,
              overflow: 'hidden',
            })}
          >
            <Box
              sx={{
                bgcolor: 'primary.main',
                height: '100%',
                width: `${Math.max(6, (method.megapixelsPerSecond / max) * 100)}%`,
              }}
            />
          </Box>
        </Stack>
      ))}
      {methods.length === 0 && (
        <Typography color="text.secondary">No benchmark sample selected.</Typography>
      )}
    </Stack>
  );
}

function BenchmarkHistory({
  records,
  selectedId,
  onSelect,
  onDelete,
}: {
  records: PerformanceBenchmarkRecord[];
  selectedId: string;
  onSelect: (id: string) => void;
  onDelete: (id: string) => void;
}) {
  return (
    <Box sx={{ maxWidth: '100%', overflowX: 'auto' }}>
      <Table size="small" sx={{ minWidth: 520 }}>
        <TableHead>
          <TableRow>
            <TableCell>Image</TableCell>
            <TableCell>Size</TableCell>
            <TableCell>Fastest</TableCell>
            <TableCell>forEach</TableCell>
            <TableCell align="right">Delete</TableCell>
          </TableRow>
        </TableHead>
        <TableBody>
          {records.map((record) => (
            <TableRow
              key={record.benchmarkId}
              hover
              selected={record.benchmarkId === selectedId}
              onClick={() => onSelect(record.benchmarkId)}
              sx={{ cursor: 'pointer' }}
            >
              <TableCell>
                <Stack spacing={0.25}>
                  <Typography variant="body2">{record.imageName}</Typography>
                  <Typography variant="caption" color="text.secondary">
                    {record.createdAt}
                  </Typography>
                </Stack>
              </TableCell>
              <TableCell>{`${record.width} x ${record.height}`}</TableCell>
              <TableCell>{record.fastestMethod}</TableCell>
              <TableCell>{record.forEachSpeedupVsPointer.toFixed(2)}x</TableCell>
              <TableCell align="right">
                <Tooltip title="Delete benchmark">
                  <IconButton
                    size="small"
                    onClick={(event) => {
                      event.stopPropagation();
                      onDelete(record.benchmarkId);
                    }}
                  >
                    <DeleteIcon fontSize="small" />
                  </IconButton>
                </Tooltip>
              </TableCell>
            </TableRow>
          ))}
          {records.length === 0 && (
            <TableRow>
              <TableCell colSpan={5}>
                <Typography color="text.secondary">No benchmark samples are available.</Typography>
              </TableCell>
            </TableRow>
          )}
        </TableBody>
      </Table>
    </Box>
  );
}

export function PerformanceLabPage() {
  const queryClient = useQueryClient();
  const [localPath, setLocalPath] = useState('');
  const [selectedResultId, setSelectedResultId] = useState('');
  const [selectedBenchmarkId, setSelectedBenchmarkId] = useState('');
  const [iterations, setIterations] = useState(5);
  const [iterationsCustomized, setIterationsCustomized] = useState(false);

  const imageResultsQuery = useQuery({
    queryKey: ['image-results'],
    queryFn: getImageResults,
    refetchInterval: 10000,
  });
  const settingsQuery = useQuery({
    queryKey: ['settings'],
    queryFn: getSettings,
    refetchInterval: 30000,
  });
  const benchmarksQuery = useQuery({
    queryKey: ['performance-benchmarks'],
    queryFn: getPerformanceBenchmarks,
    refetchInterval: 10000,
  });

  const benchmarkSettings = settingsQuery.data?.opencv.performanceBenchmark;
  const maxIterations = benchmarkSettings?.maxIterations ?? 20;
  const queryResults = imageResultsQuery.data?.results;
  const queryBenchmarks = benchmarksQuery.data?.records;
  const imageResults = useMemo(() => queryResults ?? [], [queryResults]);
  const benchmarks = useMemo(() => queryBenchmarks ?? [], [queryBenchmarks]);
  const selectedResult =
    imageResults.find((result) => result.resultId === selectedResultId) ?? imageResults[0];
  const selectedBenchmark =
    benchmarks.find((record) => record.benchmarkId === selectedBenchmarkId) ?? benchmarks[0];

  useEffect(() => {
    if (!selectedResultId && imageResults[0]) {
      setSelectedResultId(imageResults[0].resultId);
    }
  }, [imageResults, selectedResultId]);

  useEffect(() => {
    if (!selectedBenchmarkId && benchmarks[0]) {
      setSelectedBenchmarkId(benchmarks[0].benchmarkId);
    }
  }, [benchmarks, selectedBenchmarkId]);

  useEffect(() => {
    if (!benchmarkSettings) {
      return;
    }
    setIterations((current) => {
      if (iterationsCustomized) {
        return Math.max(1, Math.min(benchmarkSettings.maxIterations, current));
      }
      return benchmarkSettings.defaultIterations;
    });
  }, [benchmarkSettings, iterationsCustomized]);

  const openLocalMutation = useMutation({
    mutationFn: () => openLocalImage(localPath),
    onSuccess: (result) => {
      setSelectedResultId(result.resultId);
      void queryClient.invalidateQueries({ queryKey: ['image-results'] });
    },
  });

  const uploadMutation = useMutation({
    mutationFn: (file: File) => uploadImageFile(file),
    onSuccess: (result) => {
      setSelectedResultId(result.resultId);
      void queryClient.invalidateQueries({ queryKey: ['image-results'] });
    },
  });

  const runMutation = useMutation({
    mutationFn: () =>
      runPerformanceBenchmark({
        resultId: selectedResult?.resultId ?? '',
        iterations: Math.max(1, Math.min(maxIterations, iterations)),
      }),
    onSuccess: (response) => {
      setSelectedBenchmarkId(response.benchmark.benchmarkId);
      void queryClient.invalidateQueries({ queryKey: ['performance-benchmarks'] });
      void queryClient.invalidateQueries({ queryKey: ['jobs'] });
    },
  });

  const deleteMutation = useMutation({
    mutationFn: deletePerformanceBenchmark,
    onSuccess: () => {
      setSelectedBenchmarkId('');
      void queryClient.invalidateQueries({ queryKey: ['performance-benchmarks'] });
    },
  });

  const clearMutation = useMutation({
    mutationFn: clearPerformanceBenchmarks,
    onSuccess: () => {
      setSelectedBenchmarkId('');
      void queryClient.invalidateQueries({ queryKey: ['performance-benchmarks'] });
    },
  });

  const error =
    imageResultsQuery.error ??
    settingsQuery.error ??
    benchmarksQuery.error ??
    openLocalMutation.error ??
    uploadMutation.error ??
    runMutation.error ??
    deleteMutation.error ??
    clearMutation.error;
  const busy =
    openLocalMutation.isPending ||
    settingsQuery.isLoading ||
    uploadMutation.isPending ||
    runMutation.isPending ||
    deleteMutation.isPending ||
    clearMutation.isPending;

  return (
    <PlaceholderPage
      title="Performance Lab"
      eyebrow="OpenCV"
      status={`${benchmarks.length} samples`}
      description="Direct OpenCV pixel-access benchmark runner for image results."
    >
      <Stack spacing={2.5}>
        {error instanceof Error && <Alert severity="warning">{error.message}</Alert>}
        {busy && <LinearProgress />}

        <Card>
          <CardContent>
            <Grid container spacing={2} alignItems="center">
              <Grid item xs={12} md={4}>
                <TextField
                  select
                  label="Image Result"
                  size="small"
                  fullWidth
                  value={selectedResult?.resultId ?? ''}
                  onChange={(event) => setSelectedResultId(event.target.value)}
                >
                  <MenuItem value="" disabled>
                    No image selected
                  </MenuItem>
                  {imageResults.map((result) => (
                    <MenuItem key={result.resultId} value={result.resultId}>
                      {result.name} - {result.operation}
                    </MenuItem>
                  ))}
                </TextField>
              </Grid>
              <Grid item xs={12} md={4}>
                <TextField
                  label="Local Path"
                  size="small"
                  fullWidth
                  value={localPath}
                  onChange={(event) => setLocalPath(event.target.value)}
                />
              </Grid>
              <Grid item xs={12} sm={6} md={1.25}>
                <Button
                  fullWidth
                  variant="outlined"
                  startIcon={<FileOpenIcon />}
                  disabled={!localPath || openLocalMutation.isPending}
                  onClick={() => openLocalMutation.mutate()}
                >
                  Open
                </Button>
              </Grid>
              <Grid item xs={12} sm={6} md={1.25}>
                <Button
                  fullWidth
                  variant="outlined"
                  component="label"
                  startIcon={<UploadFileIcon />}
                >
                  Upload
                  <Box
                    component="input"
                    type="file"
                    accept="image/*"
                    hidden
                    onChange={(event) => {
                      const file = event.currentTarget.files?.[0];
                      event.currentTarget.value = '';
                      if (file) {
                        uploadMutation.mutate(file);
                      }
                    }}
                  />
                </Button>
              </Grid>
              <Grid item xs={12} md={1.5}>
                <TextField
                  label="Iterations"
                  size="small"
                  type="number"
                  fullWidth
                  value={iterations}
                  inputProps={{ min: 1, max: maxIterations }}
                  helperText={`Max ${maxIterations}`}
                  onChange={(event) => {
                    setIterationsCustomized(true);
                    setIterations(
                      Math.max(1, Math.min(maxIterations, Number(event.target.value) || 1)),
                    );
                  }}
                />
              </Grid>
            </Grid>
          </CardContent>
        </Card>

        <Grid container spacing={2}>
          <Grid item xs={12} lg={5}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack direction="row" alignItems="center" spacing={1}>
                    <SpeedIcon color="primary" />
                    <Typography variant="h6">Source</Typography>
                  </Stack>
                  {selectedResult ? (
                    <>
                      <Box
                        component="img"
                        src={absoluteImageUrl(selectedResult.previewUrl)}
                        alt={selectedResult.name}
                        sx={{
                          bgcolor: 'background.default',
                          border: 1,
                          borderColor: 'divider',
                          borderRadius: 1,
                          maxHeight: 360,
                          objectFit: 'contain',
                          width: '100%',
                        }}
                      />
                      <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                        <Chip
                          label={`${selectedResult.width} x ${selectedResult.height}`}
                          size="small"
                        />
                        <Chip label={selectedResult.operation} size="small" />
                      </Stack>
                      <Button
                        variant="contained"
                        startIcon={<PlayArrowIcon />}
                        disabled={!selectedResult || !benchmarkSettings || runMutation.isPending}
                        onClick={() => {
                          runMutation.reset();
                          runMutation.mutate();
                        }}
                      >
                        Run Benchmark
                      </Button>
                    </>
                  ) : (
                    <Typography color="text.secondary">No image result is selected.</Typography>
                  )}
                </Stack>
              </CardContent>
            </Card>
          </Grid>

          <Grid item xs={12} lg={7}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack direction="row" alignItems="center" justifyContent="space-between" gap={1}>
                    <Typography variant="h6">Comparison</Typography>
                    {selectedBenchmark && (
                      <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                        <Chip
                          label={selectedBenchmark.fastestMethod}
                          color="success"
                          size="small"
                        />
                        <Chip
                          label={`${selectedBenchmark.forEachSpeedupVsPointer.toFixed(2)}x forEach`}
                          size="small"
                        />
                      </Stack>
                    )}
                  </Stack>
                  <MethodBars methods={selectedBenchmark?.methods ?? []} />
                  {selectedBenchmark && (
                    <Box sx={{ maxWidth: '100%', overflowX: 'auto' }}>
                      <Table size="small" sx={{ minWidth: 520 }}>
                        <TableHead>
                          <TableRow>
                            <TableCell>Method</TableCell>
                            <TableCell>Average</TableCell>
                            <TableCell>Total</TableCell>
                            <TableCell>Throughput</TableCell>
                          </TableRow>
                        </TableHead>
                        <TableBody>
                          {selectedBenchmark.methods.map((method) => (
                            <TableRow key={method.name}>
                              <TableCell>{method.label}</TableCell>
                              <TableCell>{method.averageMs.toFixed(3)} ms</TableCell>
                              <TableCell>{method.totalMs.toFixed(2)} ms</TableCell>
                              <TableCell>{method.megapixelsPerSecond.toFixed(1)} MP/s</TableCell>
                            </TableRow>
                          ))}
                        </TableBody>
                      </Table>
                    </Box>
                  )}
                </Stack>
              </CardContent>
            </Card>
          </Grid>
        </Grid>

        <Card>
          <CardContent>
            <Stack spacing={2}>
              <Stack direction="row" alignItems="center" justifyContent="space-between" gap={1}>
                <Typography variant="h6">Benchmark History</Typography>
                <Button
                  color="error"
                  variant="outlined"
                  startIcon={<DeleteIcon />}
                  disabled={benchmarks.length === 0 || clearMutation.isPending}
                  onClick={() => clearMutation.mutate()}
                >
                  Clear
                </Button>
              </Stack>
              <BenchmarkHistory
                records={benchmarks}
                selectedId={selectedBenchmark?.benchmarkId ?? ''}
                onSelect={setSelectedBenchmarkId}
                onDelete={(id) => deleteMutation.mutate(id)}
              />
            </Stack>
          </CardContent>
        </Card>
      </Stack>
    </PlaceholderPage>
  );
}
