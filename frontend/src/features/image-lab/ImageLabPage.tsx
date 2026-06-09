import CompareArrowsIcon from '@mui/icons-material/CompareArrows';
import FolderOpenIcon from '@mui/icons-material/FolderOpen';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import SaveIcon from '@mui/icons-material/Save';
import UploadFileIcon from '@mui/icons-material/UploadFile';
import {
  Alert,
  Box,
  Button,
  Card,
  CardContent,
  Chip,
  Divider,
  Grid,
  MenuItem,
  Slider,
  Stack,
  TextField,
  Typography,
} from '@mui/material';
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import { useEffect, useMemo, useState, type ChangeEvent } from 'react';
import {
  absoluteImageUrl,
  getImageResult,
  getImageResults,
  processImage,
  saveImageResult,
  type ImageOperation,
  type ImageResult,
} from '../../api/imageApi';
import { getImageOpenCapabilities, openImageFromLocalPath, openImageFromUpload } from '../../runtime/fileAdapter';
import { getRuntimeMode, type RuntimeMode } from '../../runtime/runtimeMode';
import { PlaceholderPage } from '../../shared/components/PlaceholderPage';

type ParamValue = string | number;
type ImageParams = Record<string, ParamValue>;

const imageResultsQueryKey = ['image-results'];

const operationLabels: Array<{ value: ImageOperation; label: string }> = [
  { value: 'resize', label: 'Resize' },
  { value: 'crop', label: 'Crop' },
  { value: 'rotate', label: 'Rotate' },
  { value: 'flip', label: 'Flip' },
  { value: 'grayscale', label: 'Grayscale' },
  { value: 'blur', label: 'Blur' },
  { value: 'gaussianBlur', label: 'Gaussian Blur' },
  { value: 'sharpen', label: 'Sharpen' },
  { value: 'threshold', label: 'Threshold' },
  { value: 'edgeDetect', label: 'Edge Detect' },
  { value: 'contourDetect', label: 'Contour Detect' },
  { value: 'histogram', label: 'Histogram' },
  { value: 'colorConvert', label: 'Color Convert' },
  { value: 'compare', label: 'Compare Diff' },
];

function defaultParams(operation: ImageOperation, result?: ImageResult): ImageParams {
  switch (operation) {
    case 'resize':
      return { width: result?.width ?? 1280, height: result?.height ?? 720 };
    case 'crop':
      return {
        x: 0,
        y: 0,
        width: Math.max(1, Math.floor((result?.width ?? 640) * 0.75)),
        height: Math.max(1, Math.floor((result?.height ?? 480) * 0.75)),
      };
    case 'rotate':
      return { angle: 90 };
    case 'flip':
      return { direction: 'horizontal' };
    case 'blur':
    case 'gaussianBlur':
      return { kernel: 7 };
    case 'sharpen':
      return { strength: 1 };
    case 'threshold':
      return { threshold: 128 };
    case 'edgeDetect':
    case 'contourDetect':
      return { low: 80, high: 160 };
    case 'colorConvert':
      return { target: 'hsv' };
    default:
      return {};
  }
}

function mutationErrorMessage(error: unknown) {
  return error instanceof Error ? error.message : 'Image operation failed.';
}

function PreviewPanel({ title, src, emptyLabel }: { title: string; src?: string; emptyLabel: string }) {
  return (
    <Card sx={{ height: '100%' }}>
      <CardContent>
        <Stack spacing={1.5}>
          <Typography variant="h6">{title}</Typography>
          <Box
            sx={{
              alignItems: 'center',
              bgcolor: 'background.default',
              border: 1,
              borderColor: 'divider',
              borderRadius: 2,
              display: 'flex',
              justifyContent: 'center',
              minHeight: { xs: 220, md: 360 },
              overflow: 'hidden',
            }}
          >
            {src ? (
              <Box
                component="img"
                alt={title}
                src={src}
                sx={{
                  display: 'block',
                  maxHeight: { xs: 300, md: 520 },
                  maxWidth: '100%',
                  objectFit: 'contain',
                }}
              />
            ) : (
              <Typography color="text.secondary">{emptyLabel}</Typography>
            )}
          </Box>
        </Stack>
      </CardContent>
    </Card>
  );
}

export function ImageLabPage() {
  const queryClient = useQueryClient();
  const [runtimeMode, setRuntimeMode] = useState<RuntimeMode>('desktop');
  const [localPath, setLocalPath] = useState('');
  const [currentResult, setCurrentResult] = useState<ImageResult | null>(null);
  const [operation, setOperation] = useState<ImageOperation>('grayscale');
  const [params, setParams] = useState<ImageParams>(() => defaultParams('grayscale'));
  const [savePath, setSavePath] = useState<string | null>(null);

  useEffect(() => {
    setRuntimeMode(getRuntimeMode());
  }, []);

  const capabilities = useMemo(() => getImageOpenCapabilities(runtimeMode), [runtimeMode]);

  const resultQuery = useQuery({
    queryKey: [...imageResultsQueryKey, currentResult?.resultId],
    queryFn: () => getImageResult(currentResult?.resultId ?? ''),
    enabled: Boolean(currentResult?.resultId),
  });

  const resultsQuery = useQuery({
    queryKey: imageResultsQueryKey,
    queryFn: getImageResults,
    refetchInterval: 8000,
  });

  const activeResult = resultQuery.data ?? currentResult;

  const openLocalMutation = useMutation({
    mutationFn: () => openImageFromLocalPath(localPath, runtimeMode),
    onSuccess: (result) => {
      setCurrentResult(result);
      setParams(defaultParams(operation, result));
      setSavePath(null);
      void queryClient.invalidateQueries({ queryKey: imageResultsQueryKey });
    },
  });

  const uploadMutation = useMutation({
    mutationFn: openImageFromUpload,
    onSuccess: (result) => {
      setCurrentResult(result);
      setParams(defaultParams(operation, result));
      setSavePath(null);
      void queryClient.invalidateQueries({ queryKey: imageResultsQueryKey });
    },
  });

  const processMutation = useMutation({
    mutationFn: () =>
      processImage({
        resultId: activeResult?.resultId ?? '',
        operation,
        params,
      }),
    onSuccess: ({ result }) => {
      setCurrentResult(result);
      setParams(defaultParams(operation, result));
      setSavePath(null);
      void queryClient.invalidateQueries({ queryKey: imageResultsQueryKey });
      void queryClient.invalidateQueries({ queryKey: ['jobs'] });
    },
  });

  const saveMutation = useMutation({
    mutationFn: () => saveImageResult(activeResult?.resultId ?? 'missing', 'png'),
    onSuccess: (result) => {
      setSavePath(result.path);
    },
  });

  const beforeUrl = activeResult ? absoluteImageUrl(activeResult.originalPreviewUrl) : undefined;
  const afterUrl = activeResult ? absoluteImageUrl(activeResult.previewUrl) : undefined;
  const busy = openLocalMutation.isPending || uploadMutation.isPending || processMutation.isPending || saveMutation.isPending;
  const currentError = openLocalMutation.error ?? uploadMutation.error ?? processMutation.error ?? saveMutation.error;

  const setParam = (key: string, value: ParamValue) => {
    setParams((previous) => ({ ...previous, [key]: value }));
  };

  const changeOperation = (event: ChangeEvent<HTMLInputElement>) => {
    const nextOperation = event.target.value as ImageOperation;
    setOperation(nextOperation);
    setParams(defaultParams(nextOperation, activeResult ?? undefined));
  };

  const runProcess = () => {
    if (!activeResult) {
      return;
    }
    processMutation.mutate();
  };

  const renderNumberField = (key: string, label: string, minimum = 0) => (
    <TextField
      key={key}
      label={label}
      type="number"
      value={params[key] ?? minimum}
      onChange={(event) => setParam(key, Number(event.target.value))}
      inputProps={{ min: minimum }}
      size="small"
      fullWidth
    />
  );

  const renderSlider = (key: string, label: string, minimum: number, maximum: number, step = 1) => (
    <Stack key={key} spacing={0.5}>
      <Typography variant="body2" color="text.secondary">
        {label}
      </Typography>
      <Slider
        value={Number(params[key] ?? minimum)}
        min={minimum}
        max={maximum}
        step={step}
        valueLabelDisplay="auto"
        onChange={(_, value) => setParam(key, Array.isArray(value) ? value[0] : value)}
      />
    </Stack>
  );

  return (
    <PlaceholderPage
      title="Image Lab"
      eyebrow="OpenCV"
      status={activeResult ? activeResult.operation : 'Ready'}
      description="Open images, tune filters, compare before and after previews, and save processing results."
    >
      <Stack spacing={2.5}>
        {runtimeMode === 'lan' && (
          <Alert severity="info">LAN clients use upload and result inspection without arbitrary local path access.</Alert>
        )}
        {currentError && <Alert severity="error">{mutationErrorMessage(currentError)}</Alert>}
        {savePath && <Alert severity="success">Saved to {savePath}</Alert>}

        <Grid container spacing={2}>
          <Grid item xs={12} lg={4}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack direction="row" alignItems="center" justifyContent="space-between" gap={1}>
                    <Typography variant="h6">Source</Typography>
                    <Chip label={runtimeMode === 'desktop' ? 'Desktop' : 'LAN'} size="small" variant="outlined" />
                  </Stack>

                  <TextField
                    label="Local Image Path"
                    value={localPath}
                    onChange={(event) => setLocalPath(event.target.value)}
                    disabled={!capabilities.localPath || busy}
                    fullWidth
                    size="small"
                  />
                  <Stack direction={{ xs: 'column', sm: 'row' }} spacing={1}>
                    <Button
                      startIcon={<FolderOpenIcon />}
                      onClick={() => openLocalMutation.mutate()}
                      disabled={!capabilities.localPath || localPath.length === 0 || busy}
                    >
                      Open Path
                    </Button>
                    <Button component="label" variant="outlined" startIcon={<UploadFileIcon />} disabled={busy}>
                      Upload
                      <input
                        hidden
                        type="file"
                        accept="image/png,image/jpeg,image/bmp,image/webp,image/tiff"
                        onChange={(event) => {
                          const file = event.target.files?.[0];
                          if (file) {
                            uploadMutation.mutate(file);
                          }
                          event.target.value = '';
                        }}
                      />
                    </Button>
                  </Stack>

                  {activeResult && (
                    <>
                      <Divider />
                      <Stack spacing={0.75}>
                        <Typography variant="subtitle2">{activeResult.name}</Typography>
                        <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                          <Chip label={`${activeResult.width}x${activeResult.height}`} size="small" />
                          <Chip label={`${activeResult.channels} channels`} size="small" />
                          <Chip label={activeResult.sourceType} size="small" />
                        </Stack>
                      </Stack>
                    </>
                  )}
                </Stack>
              </CardContent>
            </Card>
          </Grid>

          <Grid item xs={12} lg={5}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack direction="row" alignItems="center" justifyContent="space-between" gap={1}>
                    <Typography variant="h6">Operation</Typography>
                    <CompareArrowsIcon color="primary" />
                  </Stack>
                  <TextField select label="Filter" value={operation} onChange={changeOperation} fullWidth size="small">
                    {operationLabels.map((option) => (
                      <MenuItem key={option.value} value={option.value}>
                        {option.label}
                      </MenuItem>
                    ))}
                  </TextField>

                  <Grid container spacing={1.5}>
                    {operation === 'resize' && (
                      <>
                        <Grid item xs={6}>{renderNumberField('width', 'Width', 1)}</Grid>
                        <Grid item xs={6}>{renderNumberField('height', 'Height', 1)}</Grid>
                      </>
                    )}
                    {operation === 'crop' && (
                      <>
                        <Grid item xs={6}>{renderNumberField('x', 'X')}</Grid>
                        <Grid item xs={6}>{renderNumberField('y', 'Y')}</Grid>
                        <Grid item xs={6}>{renderNumberField('width', 'Width', 1)}</Grid>
                        <Grid item xs={6}>{renderNumberField('height', 'Height', 1)}</Grid>
                      </>
                    )}
                    {operation === 'rotate' && (
                      <Grid item xs={12}>
                        <TextField
                          select
                          label="Angle"
                          value={params.angle ?? 90}
                          onChange={(event) => setParam('angle', Number(event.target.value))}
                          fullWidth
                          size="small"
                        >
                          {[90, 180, 270].map((angle) => (
                            <MenuItem key={angle} value={angle}>
                              {angle}
                            </MenuItem>
                          ))}
                        </TextField>
                      </Grid>
                    )}
                    {operation === 'flip' && (
                      <Grid item xs={12}>
                        <TextField
                          select
                          label="Direction"
                          value={params.direction ?? 'horizontal'}
                          onChange={(event) => setParam('direction', event.target.value)}
                          fullWidth
                          size="small"
                        >
                          <MenuItem value="horizontal">Horizontal</MenuItem>
                          <MenuItem value="vertical">Vertical</MenuItem>
                          <MenuItem value="both">Both</MenuItem>
                        </TextField>
                      </Grid>
                    )}
                    {(operation === 'blur' || operation === 'gaussianBlur') && (
                      <Grid item xs={12}>{renderSlider('kernel', 'Kernel', 1, 31, 2)}</Grid>
                    )}
                    {operation === 'sharpen' && (
                      <Grid item xs={12}>{renderSlider('strength', 'Strength', 0.2, 4, 0.1)}</Grid>
                    )}
                    {operation === 'threshold' && (
                      <Grid item xs={12}>{renderSlider('threshold', 'Threshold', 0, 255)}</Grid>
                    )}
                    {(operation === 'edgeDetect' || operation === 'contourDetect') && (
                      <>
                        <Grid item xs={12}>{renderSlider('low', 'Low', 0, 255)}</Grid>
                        <Grid item xs={12}>{renderSlider('high', 'High', 0, 255)}</Grid>
                      </>
                    )}
                    {operation === 'colorConvert' && (
                      <Grid item xs={12}>
                        <TextField
                          select
                          label="Target"
                          value={params.target ?? 'hsv'}
                          onChange={(event) => setParam('target', event.target.value)}
                          fullWidth
                          size="small"
                        >
                          <MenuItem value="hsv">HSV</MenuItem>
                          <MenuItem value="lab">Lab</MenuItem>
                          <MenuItem value="gray">Gray</MenuItem>
                        </TextField>
                      </Grid>
                    )}
                  </Grid>

                  <Stack direction={{ xs: 'column', sm: 'row' }} spacing={1}>
                    <Button startIcon={<PlayArrowIcon />} onClick={runProcess} disabled={!activeResult || busy}>
                      Apply
                    </Button>
                    <Button
                      variant="outlined"
                      startIcon={<SaveIcon />}
                      onClick={() => saveMutation.mutate()}
                      disabled={!activeResult || busy}
                    >
                      Save PNG
                    </Button>
                  </Stack>
                </Stack>
              </CardContent>
            </Card>
          </Grid>

          <Grid item xs={12} lg={3}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={1.5}>
                  <Typography variant="h6">Results</Typography>
                  <Stack spacing={1} sx={{ maxHeight: 250, overflowY: 'auto', pr: 0.5 }}>
                    {(resultsQuery.data?.results ?? []).map((result) => (
                      <Button
                        key={result.resultId}
                        variant={result.resultId === activeResult?.resultId ? 'contained' : 'outlined'}
                        onClick={() => setCurrentResult(result)}
                        sx={{ justifyContent: 'flex-start', textAlign: 'left' }}
                      >
                        {result.operation} - {result.width}x{result.height}
                      </Button>
                    ))}
                    {resultsQuery.data?.results.length === 0 && (
                      <Typography color="text.secondary">No image results yet.</Typography>
                    )}
                  </Stack>
                </Stack>
              </CardContent>
            </Card>
          </Grid>
        </Grid>

        <Grid container spacing={2}>
          <Grid item xs={12} md={6}>
            <PreviewPanel title="Before" src={beforeUrl} emptyLabel="Open an image to preview the source." />
          </Grid>
          <Grid item xs={12} md={6}>
            <PreviewPanel title="After" src={afterUrl} emptyLabel="Apply a filter to preview the result." />
          </Grid>
        </Grid>
      </Stack>
    </PlaceholderPage>
  );
}
