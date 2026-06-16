import CompareArrowsIcon from '@mui/icons-material/CompareArrows';
import DeleteIcon from '@mui/icons-material/Delete';
import FolderOpenIcon from '@mui/icons-material/FolderOpen';
import PhotoCameraIcon from '@mui/icons-material/PhotoCamera';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import QrCodeScannerIcon from '@mui/icons-material/QrCodeScanner';
import SaveIcon from '@mui/icons-material/Save';
import UndoIcon from '@mui/icons-material/Undo';
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
  Stack,
  TextField,
  Typography,
} from '@mui/material';
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import { useEffect, useMemo, useState, type ChangeEvent } from 'react';
import {
  absoluteImageUrl,
  createCalibrationResult,
  deleteImageResult,
  getCalibrationResults,
  getImageResult,
  getImageResults,
  processImage,
  saveImageResult,
  type ImageOperation,
} from '../../api/imageApi';
import {
  getImageOpenCapabilities,
  openImageFromLocalPath,
  openImageFromUpload,
} from '../../runtime/fileAdapter';
import { getRuntimeMode, type RuntimeMode } from '../../runtime/runtimeMode';
import { PlaceholderPage } from '../../shared/components/PlaceholderPage';
import { useImageLabStore } from '../../store/useImageLabStore';
import { ImageOperationControls } from './ImageOperationControls';
import {
  advancedRenderOperations,
  alignmentUtilityOperations,
  defaultParams,
  labelForOperation,
  operationLabels,
  sampleBoardOperations,
  shapeAnalysisOperations,
} from './imageOperations';

const imageResultsQueryKey = ['image-results'];

function mutationErrorMessage(error: unknown) {
  return error instanceof Error ? error.message : 'Image operation failed.';
}

function PreviewPanel({
  title,
  src,
  emptyLabel,
}: {
  title: string;
  src?: string;
  emptyLabel: string;
}) {
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
  const {
    localPath,
    currentResult,
    operation,
    params,
    savePath,
    setLocalPath,
    setCurrentResult,
    setOperation,
    setParams,
    setParam,
    setSavePath,
  } = useImageLabStore();

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

  const calibrationQuery = useQuery({
    queryKey: ['calibration-results'],
    queryFn: getCalibrationResults,
    refetchInterval: 15000,
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

  const calibrationMutation = useMutation({
    mutationFn: () =>
      createCalibrationResult({
        resultId: activeResult?.resultId ?? '',
        boardWidth: Number(params.boardWidth ?? 9),
        boardHeight: Number(params.boardHeight ?? 6),
        squareSize: Number(params.squareSize ?? 1),
      }),
    onSuccess: () => {
      void queryClient.invalidateQueries({ queryKey: ['calibration-results'] });
    },
  });

  const deleteMutation = useMutation({
    mutationFn: () => deleteImageResult(activeResult?.resultId ?? ''),
    onSuccess: () => {
      setCurrentResult(null);
      setSavePath(null);
      void queryClient.invalidateQueries({ queryKey: imageResultsQueryKey });
    },
  });

  const beforeUrl = activeResult ? absoluteImageUrl(activeResult.originalPreviewUrl) : undefined;
  const afterUrl = activeResult ? absoluteImageUrl(activeResult.previewUrl) : undefined;
  const busy =
    openLocalMutation.isPending ||
    uploadMutation.isPending ||
    processMutation.isPending ||
    calibrationMutation.isPending ||
    saveMutation.isPending ||
    deleteMutation.isPending;
  const currentError =
    openLocalMutation.error ??
    uploadMutation.error ??
    processMutation.error ??
    calibrationMutation.error ??
    saveMutation.error ??
    deleteMutation.error;

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

  const revertToSource = () => {
    if (!activeResult) {
      return;
    }

    const sourceResult = (resultsQuery.data?.results ?? []).find(
      (result) =>
        result.operation === 'open' &&
        result.name === activeResult.name &&
        result.sourceType === activeResult.sourceType &&
        result.sourcePath === activeResult.sourcePath,
    );

    setCurrentResult(sourceResult ?? activeResult);
    setOperation('grayscale');
    setParams(defaultParams('grayscale', sourceResult ?? activeResult));
    setSavePath(null);
  };

  return (
    <PlaceholderPage
      title="Image Lab"
      eyebrow="OpenCV"
      status={activeResult ? activeResult.operation : 'Ready'}
      description="Open images, tune filters, compare before and after previews, and save processing results."
    >
      <Stack spacing={2.5}>
        {runtimeMode === 'lan' && (
          <Alert severity="info">
            LAN clients use upload and result inspection without arbitrary local path access.
          </Alert>
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
                    <Chip
                      label={runtimeMode === 'desktop' ? 'Desktop' : 'LAN'}
                      size="small"
                      variant="outlined"
                    />
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
                    <Button
                      component="label"
                      variant="outlined"
                      startIcon={<UploadFileIcon />}
                      disabled={busy}
                    >
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
                          <Chip
                            label={`${activeResult.width}x${activeResult.height}`}
                            size="small"
                          />
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
                  <TextField
                    select
                    label="Filter"
                    value={operation}
                    onChange={changeOperation}
                    fullWidth
                    size="small"
                  >
                    {operationLabels.map((option) => (
                      <MenuItem key={option.value} value={option.value}>
                        {option.label}
                      </MenuItem>
                    ))}
                  </TextField>

                  <ImageOperationControls
                    operation={operation}
                    params={params}
                    setParam={setParam}
                  />

                  <Stack direction={{ xs: 'column', sm: 'row' }} spacing={1}>
                    <Button
                      startIcon={<UndoIcon />}
                      variant="outlined"
                      onClick={revertToSource}
                      disabled={!activeResult || busy}
                    >
                      Revert
                    </Button>
                    <Button
                      startIcon={<PlayArrowIcon />}
                      onClick={runProcess}
                      disabled={!activeResult || busy}
                    >
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
                    <Button
                      variant="outlined"
                      startIcon={<PhotoCameraIcon />}
                      onClick={() => calibrationMutation.mutate()}
                      disabled={!activeResult || busy}
                    >
                      Calibrate
                    </Button>
                    <Button
                      variant="outlined"
                      color="error"
                      startIcon={<DeleteIcon />}
                      onClick={() => deleteMutation.mutate()}
                      disabled={!activeResult || busy}
                    >
                      Delete
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
                        variant={
                          result.resultId === activeResult?.resultId ? 'contained' : 'outlined'
                        }
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
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={1.5}>
                  <Stack direction="row" alignItems="center" justifyContent="space-between" gap={1}>
                    <Typography variant="h6">Alignment & QR Utilities</Typography>
                    <QrCodeScannerIcon color="primary" />
                  </Stack>
                  <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                    {alignmentUtilityOperations.map((utility) => (
                      <Chip
                        key={utility}
                        label={labelForOperation(utility)}
                        color={operation === utility ? 'primary' : 'default'}
                        onClick={() => {
                          setOperation(utility);
                          setParams(defaultParams(utility, activeResult ?? undefined));
                        }}
                      />
                    ))}
                  </Stack>
                </Stack>
              </CardContent>
            </Card>
          </Grid>
          <Grid item xs={12} md={6}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={1.5}>
                  <Typography variant="h6">Shape Analysis</Typography>
                  <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                    {shapeAnalysisOperations.map((utility) => (
                      <Chip
                        key={utility}
                        label={labelForOperation(utility)}
                        color={operation === utility ? 'primary' : 'default'}
                        onClick={() => {
                          setOperation(utility);
                          setParams(defaultParams(utility, activeResult ?? undefined));
                        }}
                      />
                    ))}
                  </Stack>
                  {activeResult?.metadata?.shape && (
                    <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                      <Chip
                        label={`Shapes ${activeResult.metadata.shape.shapeCount ?? 0}`}
                        size="small"
                        variant="outlined"
                      />
                      <Chip
                        label={`Contours ${activeResult.metadata.shape.contourCount ?? 0}`}
                        size="small"
                        variant="outlined"
                      />
                      {activeResult.metadata.shape.largestArea !== undefined && (
                        <Chip
                          label={`Largest ${activeResult.metadata.shape.largestArea.toFixed(1)}`}
                          size="small"
                          variant="outlined"
                        />
                      )}
                    </Stack>
                  )}
                </Stack>
              </CardContent>
            </Card>
          </Grid>
          <Grid item xs={12} md={6}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={1.5}>
                  <Typography variant="h6">Advanced Rendering</Typography>
                  <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                    {advancedRenderOperations.map((utility) => (
                      <Chip
                        key={utility}
                        label={labelForOperation(utility)}
                        color={operation === utility ? 'primary' : 'default'}
                        onClick={() => {
                          setOperation(utility);
                          setParams(defaultParams(utility, activeResult ?? undefined));
                        }}
                      />
                    ))}
                  </Stack>
                  {activeResult?.metadata?.composition && (
                    <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                      <Chip
                        label={String(activeResult.metadata.composition.operation ?? 'composition')}
                        size="small"
                        variant="outlined"
                      />
                    </Stack>
                  )}
                </Stack>
              </CardContent>
            </Card>
          </Grid>
          <Grid item xs={12} md={6}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={1.5}>
                  <Typography variant="h6">README Sample Board</Typography>
                  <Typography variant="body2" color="text.secondary">
                    Generate the same 2x3 OpenCV comparison image shown in the README from the
                    current Image Lab result.
                  </Typography>
                  <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                    {sampleBoardOperations.map((utility) => (
                      <Chip
                        key={utility}
                        label={labelForOperation(utility)}
                        color={operation === utility ? 'primary' : 'default'}
                        onClick={() => {
                          setOperation(utility);
                          setParams(defaultParams(utility, activeResult ?? undefined));
                        }}
                      />
                    ))}
                  </Stack>
                  {activeResult?.metadata?.sampleBoard && (
                    <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                      <Chip
                        label={`Contours ${activeResult.metadata.sampleBoard.contourCount ?? 0}`}
                        size="small"
                        variant="outlined"
                      />
                      <Chip
                        label={`ORB ${activeResult.metadata.sampleBoard.orbKeypoints ?? 0}`}
                        size="small"
                        variant="outlined"
                      />
                    </Stack>
                  )}
                </Stack>
              </CardContent>
            </Card>
          </Grid>
          <Grid item xs={12} md={6}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={1.5}>
                  <Typography variant="h6">Calibration Results</Typography>
                  <Stack spacing={1} sx={{ maxHeight: 180, overflowY: 'auto', pr: 0.5 }}>
                    {(calibrationQuery.data?.records ?? []).slice(0, 4).map((record) => (
                      <Stack
                        key={record.calibrationId}
                        direction="row"
                        alignItems="center"
                        justifyContent="space-between"
                        gap={1}
                      >
                        <Typography variant="body2" noWrap>
                          {record.imageName}
                        </Typography>
                        <Chip
                          label={`RMS ${record.rmsReprojectionError.toFixed(3)}`}
                          size="small"
                          variant="outlined"
                        />
                      </Stack>
                    ))}
                    {calibrationQuery.data?.records.length === 0 && (
                      <Typography color="text.secondary">No calibration artifacts yet.</Typography>
                    )}
                  </Stack>
                </Stack>
              </CardContent>
            </Card>
          </Grid>
        </Grid>

        <Grid container spacing={2}>
          <Grid item xs={12} md={6}>
            <PreviewPanel
              title="Before"
              src={beforeUrl}
              emptyLabel="Open an image to preview the source."
            />
          </Grid>
          <Grid item xs={12} md={6}>
            <PreviewPanel
              title="After"
              src={afterUrl}
              emptyLabel="Apply a filter to preview the result."
            />
          </Grid>
        </Grid>
      </Stack>
    </PlaceholderPage>
  );
}
