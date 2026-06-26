import CenterFocusStrongIcon from '@mui/icons-material/CenterFocusStrong';
import DeleteIcon from '@mui/icons-material/Delete';
import FileOpenIcon from '@mui/icons-material/FileOpen';
import ImageSearchIcon from '@mui/icons-material/ImageSearch';
import RestartAltIcon from '@mui/icons-material/RestartAlt';
import SaveIcon from '@mui/icons-material/Save';
import TextFieldsIcon from '@mui/icons-material/TextFields';
import TuneIcon from '@mui/icons-material/Tune';
import UploadFileIcon from '@mui/icons-material/UploadFile';
import {
  Alert,
  Autocomplete,
  Box,
  Button,
  Card,
  CardContent,
  Chip,
  CircularProgress,
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
import { alpha, useTheme } from '@mui/material/styles';
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import {
  extractContourCandidate,
  getContourCandidates,
  getContourOcrLanguages,
  previewContourCandidate,
  recognizeContourCandidateText,
  type ContourCandidate,
  type ContourDetectionParams,
  type ContourOcrResponse,
} from '../../api/contourApi';
import {
  absoluteImageUrl,
  deleteImageResult,
  getImageResults,
  openLocalImage,
  saveImageResult,
  uploadImageFile,
  type ImageResult,
  type SaveImageResponse,
} from '../../api/imageApi';
import { getSettings } from '../../api/settingsApi';
import { PlaceholderPage } from '../../shared/components/PlaceholderPage';

const OCR_PAGE_SEG_MODES = [
  { value: 'auto', label: 'Auto' },
  { value: 'single-block', label: 'Single Block' },
  { value: 'single-line', label: 'Single Line' },
  { value: 'single-word', label: 'Single Word' },
  { value: 'sparse', label: 'Sparse Text' },
];

function formatPercent(value: number) {
  return `${(value * 100).toFixed(1)}%`;
}

function polygonPoints(candidate: ContourCandidate) {
  return candidate.points.map((point) => `${point.x},${point.y}`).join(' ');
}

function CandidateOverlay({
  result,
  candidates,
  selectedCandidateId,
  onSelect,
}: {
  result: ImageResult;
  candidates: ContourCandidate[];
  selectedCandidateId: string;
  onSelect: (candidateId: string) => void;
}) {
  const theme = useTheme();

  return (
    <Box
      sx={{
        bgcolor: 'background.default',
        border: 1,
        borderColor: 'divider',
        borderRadius: 1,
        overflow: 'hidden',
        position: 'relative',
        width: '100%',
        aspectRatio: `${Math.max(1, result.width)} / ${Math.max(1, result.height)}`,
      }}
    >
      <Box
        component="img"
        src={absoluteImageUrl(result.previewUrl)}
        alt={result.name}
        sx={{
          display: 'block',
          height: '100%',
          objectFit: 'contain',
          width: '100%',
        }}
      />
      <Box
        component="svg"
        viewBox={`0 0 ${Math.max(1, result.width)} ${Math.max(1, result.height)}`}
        sx={{
          inset: 0,
          position: 'absolute',
          touchAction: 'manipulation',
        }}
      >
        {candidates.map((candidate) => {
          const selected = candidate.candidateId === selectedCandidateId;
          return (
            <polygon
              key={candidate.candidateId}
              points={polygonPoints(candidate)}
              role="button"
              tabIndex={0}
              onClick={() => onSelect(candidate.candidateId)}
              onKeyDown={(event) => {
                if (event.key === 'Enter' || event.key === ' ') {
                  onSelect(candidate.candidateId);
                }
              }}
              fill={
                selected
                  ? alpha(theme.palette.success.main, 0.24)
                  : alpha(theme.palette.primary.main, 0.14)
              }
              stroke={selected ? theme.palette.success.main : theme.palette.primary.main}
              strokeWidth={selected ? 5 : 3}
              style={{ cursor: 'pointer', outline: 'none' }}
            />
          );
        })}
      </Box>
    </Box>
  );
}

function CandidateList({
  candidates,
  selectedCandidateId,
  onSelect,
  onClear,
}: {
  candidates: ContourCandidate[];
  selectedCandidateId: string;
  onSelect: (candidateId: string) => void;
  onClear: () => void;
}) {
  return (
    <Stack spacing={1}>
      <Stack direction="row" alignItems="center" justifyContent="space-between" gap={1}>
        <Typography variant="subtitle2">Candidates</Typography>
        <Button
          size="small"
          color="inherit"
          startIcon={<RestartAltIcon />}
          disabled={candidates.length === 0}
          onClick={onClear}
        >
          Clear
        </Button>
      </Stack>
      {candidates.map((candidate) => (
        <Button
          key={candidate.candidateId}
          variant={candidate.candidateId === selectedCandidateId ? 'contained' : 'outlined'}
          color={candidate.candidateId === selectedCandidateId ? 'success' : 'primary'}
          onClick={() => onSelect(candidate.candidateId)}
          sx={{ justifyContent: 'space-between', minHeight: 48 }}
        >
          <Box component="span">#{candidate.index}</Box>
          <Box component="span">{candidate.source ?? 'contour'}</Box>
          <Box component="span">{formatPercent(candidate.areaRatio)}</Box>
        </Button>
      ))}
      {candidates.length === 0 && (
        <Typography color="text.secondary">No contour candidates are available.</Typography>
      )}
    </Stack>
  );
}

function DetectionControls({
  params,
  onChange,
  onReset,
}: {
  params: ContourDetectionParams;
  onChange: (params: ContourDetectionParams) => void;
  onReset: () => void;
}) {
  const update = <Key extends keyof ContourDetectionParams>(
    key: Key,
    value: ContourDetectionParams[Key],
  ) => onChange({ ...params, [key]: value });

  return (
    <Card>
      <CardContent>
        <Stack spacing={2}>
          <Stack direction="row" spacing={1} alignItems="center">
            <TuneIcon color="primary" />
            <Typography variant="h6">Detection Parameters</Typography>
            <Button size="small" startIcon={<RestartAltIcon />} onClick={onReset}>
              Reset
            </Button>
          </Stack>
          <Grid container spacing={1.5}>
            <Grid item xs={6} md={2}>
              <TextField
                label="Max"
                size="small"
                type="number"
                fullWidth
                value={params.maxCandidates}
                inputProps={{ min: 1, max: 80 }}
                onChange={(event) =>
                  update(
                    'maxCandidates',
                    Math.max(1, Math.min(80, Number(event.target.value) || 1)),
                  )
                }
              />
            </Grid>
            <Grid item xs={6} md={2}>
              <TextField
                label="Canny Low"
                size="small"
                type="number"
                fullWidth
                value={params.low}
                inputProps={{ min: 1, max: 255 }}
                onChange={(event) =>
                  update('low', Math.max(1, Math.min(255, Number(event.target.value) || 1)))
                }
              />
            </Grid>
            <Grid item xs={6} md={2}>
              <TextField
                label="Canny High"
                size="small"
                type="number"
                fullWidth
                value={params.high}
                inputProps={{ min: 1, max: 255 }}
                onChange={(event) =>
                  update('high', Math.max(1, Math.min(255, Number(event.target.value) || 1)))
                }
              />
            </Grid>
            <Grid item xs={6} md={2}>
              <TextField
                label="Min Area %"
                size="small"
                type="number"
                fullWidth
                value={(params.minAreaRatio * 100).toFixed(2)}
                inputProps={{ min: 0.01, max: 50, step: 0.01 }}
                onChange={(event) =>
                  update(
                    'minAreaRatio',
                    Math.max(0.0001, Math.min(0.5, (Number(event.target.value) || 0.01) / 100)),
                  )
                }
              />
            </Grid>
            <Grid item xs={6} md={2}>
              <TextField
                label="Epsilon"
                size="small"
                type="number"
                fullWidth
                value={params.epsilon}
                inputProps={{ min: 0.005, max: 0.12, step: 0.001 }}
                onChange={(event) =>
                  update(
                    'epsilon',
                    Math.max(0.005, Math.min(0.12, Number(event.target.value) || 0.024)),
                  )
                }
              />
            </Grid>
            <Grid item xs={6} md={2}>
              <TextField
                select
                label="Retrieval"
                size="small"
                fullWidth
                value={params.retrieval}
                onChange={(event) => update('retrieval', event.target.value as 'list' | 'external')}
              >
                <MenuItem value="list">List</MenuItem>
                <MenuItem value="external">External</MenuItem>
              </TextField>
            </Grid>
          </Grid>
        </Stack>
      </CardContent>
    </Card>
  );
}

function OcrResultPanel({ result }: { result: ContourOcrResponse | null }) {
  if (!result) {
    return (
      <Typography color="text.secondary">
        Select a contour and run OCR to recognize high-contrast printed text.
      </Typography>
    );
  }

  return (
    <Stack spacing={1.25}>
      <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
        <Chip label={`Confidence ${(result.confidence * 100).toFixed(0)}%`} size="small" />
        <Chip label={`${result.lineCount} lines`} size="small" />
        <Chip label={`${result.wordCount} words`} size="small" />
        <Chip label={result.method.language} size="small" />
      </Stack>
      <Box
        sx={{
          bgcolor: 'background.default',
          border: 1,
          borderColor: 'divider',
          borderRadius: 1,
          minHeight: 72,
          p: 1.5,
        }}
      >
        <Typography
          component="pre"
          sx={{
            fontFamily: 'monospace',
            fontSize: '1rem',
            m: 0,
            whiteSpace: 'pre-wrap',
            wordBreak: 'break-word',
          }}
        >
          {result.text || '(no text recognized)'}
        </Typography>
      </Box>
      {result.lines.length > 0 && (
        <Stack spacing={0.5}>
          {result.lines.map((line, index) => (
            <Stack key={`${line.text}-${index}`} direction="row" spacing={1} alignItems="center">
              <Chip label={`L${index + 1}`} size="small" variant="outlined" />
              <Typography variant="body2" sx={{ fontFamily: 'monospace' }}>
                {line.text}
              </Typography>
              <Typography variant="caption" color="text.secondary">
                {(line.confidence * 100).toFixed(0)}%
              </Typography>
            </Stack>
          ))}
        </Stack>
      )}
      <Typography variant="caption" color="text.secondary">
        {result.method.engine} - {result.method.pageSegMode} - {result.method.preprocessing}
      </Typography>
    </Stack>
  );
}

function ExtractionHistory({
  results,
  selectedId,
  onSelect,
  onDelete,
  onSave,
  savingResultId,
}: {
  results: ImageResult[];
  selectedId: string;
  onSelect: (result: ImageResult) => void;
  onDelete: (resultId: string) => void;
  onSave: (resultId: string) => void;
  savingResultId: string;
}) {
  return (
    <Table size="small">
      <TableHead>
        <TableRow>
          <TableCell>Preview</TableCell>
          <TableCell>Result</TableCell>
          <TableCell>Size</TableCell>
          <TableCell align="right">Actions</TableCell>
        </TableRow>
      </TableHead>
      <TableBody>
        {results.map((result) => (
          <TableRow
            key={result.resultId}
            hover
            selected={result.resultId === selectedId}
            onClick={() => onSelect(result)}
            sx={{ cursor: 'pointer' }}
          >
            <TableCell sx={{ width: 88 }}>
              <Box
                component="img"
                src={absoluteImageUrl(result.previewUrl)}
                alt={result.name}
                sx={{
                  bgcolor: 'background.default',
                  border: 1,
                  borderColor: 'divider',
                  borderRadius: 1,
                  height: 52,
                  objectFit: 'contain',
                  width: 72,
                }}
              />
            </TableCell>
            <TableCell>
              <Stack spacing={0.25}>
                <Typography variant="body2">{result.name}</Typography>
                <Typography variant="caption" color="text.secondary">
                  {result.createdAt}
                </Typography>
              </Stack>
            </TableCell>
            <TableCell>{`${result.width} x ${result.height}`}</TableCell>
            <TableCell align="right">
              <Stack direction="row" justifyContent="flex-end" spacing={0.5}>
                <Tooltip title="Save extracted image">
                  <span>
                    <IconButton
                      size="small"
                      disabled={savingResultId === result.resultId}
                      onClick={(event) => {
                        event.stopPropagation();
                        onSave(result.resultId);
                      }}
                    >
                      <SaveIcon fontSize="small" />
                    </IconButton>
                  </span>
                </Tooltip>
                <Tooltip title="Delete extracted result">
                  <IconButton
                    size="small"
                    onClick={(event) => {
                      event.stopPropagation();
                      onDelete(result.resultId);
                    }}
                  >
                    <DeleteIcon fontSize="small" />
                  </IconButton>
                </Tooltip>
              </Stack>
            </TableCell>
          </TableRow>
        ))}
        {results.length === 0 && (
          <TableRow>
            <TableCell colSpan={4}>
              <Typography color="text.secondary">No perspective extraction history yet.</Typography>
            </TableCell>
          </TableRow>
        )}
      </TableBody>
    </Table>
  );
}

export function ContourExtractorPage() {
  const queryClient = useQueryClient();
  const previewUrlRef = useRef<string | null>(null);
  const [localPath, setLocalPath] = useState('');
  const [selectedResultId, setSelectedResultId] = useState('');
  const [selectedCandidateId, setSelectedCandidateId] = useState('');
  const [previewUrl, setPreviewUrl] = useState('');
  const [latestExtracted, setLatestExtracted] = useState<ImageResult | null>(null);
  const [latestOcr, setLatestOcr] = useState<ContourOcrResponse | null>(null);
  const [lastSaved, setLastSaved] = useState<SaveImageResponse | null>(null);
  const [detectionParams, setDetectionParams] = useState<ContourDetectionParams | null>(null);
  const [detectionParamsCustomized, setDetectionParamsCustomized] = useState(false);
  const [ocrLanguages, setOcrLanguages] = useState<string[]>(['eng', 'kor']);
  const [ocrLanguagesCustomized, setOcrLanguagesCustomized] = useState(false);
  const [ocrPageSegMode, setOcrPageSegMode] = useState('auto');

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
  const ocrLanguagesQuery = useQuery({
    queryKey: ['contour-ocr-languages'],
    queryFn: getContourOcrLanguages,
    refetchInterval: 30000,
  });

  const queryResults = imageResultsQuery.data?.results;
  const results = useMemo(() => queryResults ?? [], [queryResults]);
  const extractionResults = useMemo(
    () => results.filter((result) => result.operation === 'perspectiveExtract'),
    [results],
  );
  const selectedResult = useMemo(
    () => results.find((result) => result.resultId === selectedResultId) ?? results[0],
    [results, selectedResultId],
  );
  const effectiveResultId = selectedResult?.resultId ?? '';

  useEffect(() => {
    if (!selectedResultId && results[0]) {
      setSelectedResultId(results[0].resultId);
    }
  }, [results, selectedResultId]);

  const contourDefaults = settingsQuery.data?.opencv.contourDetection ?? null;

  useEffect(() => {
    if (contourDefaults && !detectionParamsCustomized) {
      setDetectionParams(contourDefaults);
    }
  }, [contourDefaults, detectionParamsCustomized]);

  useEffect(() => {
    const defaults = ocrLanguagesQuery.data?.defaultLanguages;
    if (defaults?.length && !ocrLanguagesCustomized) {
      setOcrLanguages(defaults);
    }
  }, [ocrLanguagesCustomized, ocrLanguagesQuery.data?.defaultLanguages]);

  useEffect(
    () => () => {
      if (previewUrlRef.current) {
        URL.revokeObjectURL(previewUrlRef.current);
      }
    },
    [],
  );

  const openLocalMutation = useMutation({
    mutationFn: () => openLocalImage(localPath),
    onSuccess: (result) => {
      setSelectedResultId(result.resultId);
      setLatestExtracted(null);
      void queryClient.invalidateQueries({ queryKey: ['image-results'] });
    },
  });

  const uploadMutation = useMutation({
    mutationFn: (file: File) => uploadImageFile(file),
    onSuccess: (result) => {
      setSelectedResultId(result.resultId);
      setLatestExtracted(null);
      void queryClient.invalidateQueries({ queryKey: ['image-results'] });
    },
  });

  const previewMutation = useMutation({
    mutationFn: (candidate: ContourCandidate) =>
      previewContourCandidate({ resultId: effectiveResultId, candidate }),
    onSuccess: (blob) => {
      if (previewUrlRef.current) {
        URL.revokeObjectURL(previewUrlRef.current);
      }
      const nextUrl = URL.createObjectURL(blob);
      previewUrlRef.current = nextUrl;
      setPreviewUrl(nextUrl);
    },
  });

  const clearPreview = useCallback(() => {
    setPreviewUrl('');
    if (previewUrlRef.current) {
      URL.revokeObjectURL(previewUrlRef.current);
      previewUrlRef.current = null;
    }
  }, []);

  const candidatesMutation = useMutation({
    mutationFn: () => {
      if (!detectionParams) {
        throw new Error('Contour detection settings are still loading.');
      }
      return getContourCandidates(effectiveResultId, detectionParams);
    },
    onSuccess: (response) => {
      const first = response.candidates[0];
      setSelectedCandidateId(first?.candidateId ?? '');
      if (first) {
        previewMutation.mutate(first);
      } else {
        clearPreview();
      }
    },
  });

  const candidates =
    candidatesMutation.data?.source.resultId === effectiveResultId
      ? candidatesMutation.data.candidates
      : [];
  const selectedCandidate =
    candidates.find((candidate) => candidate.candidateId === selectedCandidateId) ?? candidates[0];

  useEffect(() => {
    setSelectedCandidateId('');
    setLatestExtracted(null);
    setLatestOcr(null);
    clearPreview();
  }, [clearPreview, effectiveResultId]);

  const selectCandidate = (candidateId: string) => {
    setSelectedCandidateId(candidateId);
    setLatestOcr(null);
    const candidate = candidates.find((item) => item.candidateId === candidateId);
    if (candidate) {
      previewMutation.mutate(candidate);
    }
  };

  const extractMutation = useMutation({
    mutationFn: () => {
      if (!selectedCandidate) {
        throw new Error('Select a contour candidate before extraction.');
      }
      return extractContourCandidate({ resultId: effectiveResultId, candidate: selectedCandidate });
    },
    onSuccess: (response) => {
      setLatestExtracted(response.result);
      setLastSaved(null);
      void queryClient.invalidateQueries({ queryKey: ['image-results'] });
      void queryClient.invalidateQueries({ queryKey: ['jobs'] });
    },
  });

  const ocrMutation = useMutation({
    mutationFn: () => {
      if (!selectedCandidate) {
        throw new Error('Select a contour candidate before OCR.');
      }
      return recognizeContourCandidateText({
        resultId: effectiveResultId,
        candidate: selectedCandidate,
        languages: ocrLanguages,
        pageSegMode: ocrPageSegMode,
      });
    },
    onSuccess: (response) => {
      setLatestOcr(response);
    },
  });

  const saveExtractionMutation = useMutation({
    mutationFn: (resultId: string) => saveImageResult(resultId, 'png'),
    onSuccess: (response) => {
      setLastSaved(response);
    },
  });

  const deleteExtractionMutation = useMutation({
    mutationFn: deleteImageResult,
    onSuccess: (_response, resultId) => {
      if (latestExtracted?.resultId === resultId) {
        setLatestExtracted(null);
      }
      void queryClient.invalidateQueries({ queryKey: ['image-results'] });
    },
  });

  const clearCandidates = () => {
    candidatesMutation.reset();
    setSelectedCandidateId('');
    setLatestOcr(null);
    clearPreview();
  };

  const handleDetectionParamsChange = (nextParams: ContourDetectionParams) => {
    setDetectionParamsCustomized(true);
    setDetectionParams(nextParams);
  };

  const resetDetectionParams = () => {
    if (contourDefaults) {
      setDetectionParamsCustomized(false);
      setDetectionParams(contourDefaults);
    }
  };

  const busy =
    openLocalMutation.isPending ||
    uploadMutation.isPending ||
    settingsQuery.isLoading ||
    candidatesMutation.isPending ||
    previewMutation.isPending ||
    ocrMutation.isPending ||
    extractMutation.isPending ||
    saveExtractionMutation.isPending ||
    deleteExtractionMutation.isPending;
  const error =
    imageResultsQuery.error ??
    settingsQuery.error ??
    ocrLanguagesQuery.error ??
    openLocalMutation.error ??
    uploadMutation.error ??
    candidatesMutation.error ??
    previewMutation.error ??
    ocrMutation.error ??
    extractMutation.error ??
    saveExtractionMutation.error ??
    deleteExtractionMutation.error;

  return (
    <PlaceholderPage
      title="Contour Extractor"
      eyebrow="Vision Tools"
      status={selectedResult ? `${selectedResult.width} x ${selectedResult.height}` : 'No image'}
      description="Interactive quadrilateral detection and perspective extraction for document, label, and part images."
    >
      <Stack spacing={2.5}>
        {error instanceof Error && <Alert severity="warning">{error.message}</Alert>}
        {lastSaved && <Alert severity="success">Saved extracted image as {lastSaved.path}</Alert>}
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
                  value={effectiveResultId}
                  onChange={(event) => setSelectedResultId(event.target.value)}
                >
                  <MenuItem value="" disabled>
                    No image result selected
                  </MenuItem>
                  {results.map((result) => (
                    <MenuItem key={result.resultId} value={result.resultId}>
                      {result.name} - {result.operation}
                    </MenuItem>
                  ))}
                </TextField>
              </Grid>
              <Grid item xs={12} md={5}>
                <TextField
                  label="Local Path"
                  size="small"
                  fullWidth
                  value={localPath}
                  onChange={(event) => setLocalPath(event.target.value)}
                />
              </Grid>
              <Grid item xs={12} sm={6} md={1.5}>
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
              <Grid item xs={12} sm={6} md={1.5}>
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
            </Grid>
          </CardContent>
        </Card>

        {detectionParams ? (
          <DetectionControls
            params={detectionParams}
            onChange={handleDetectionParamsChange}
            onReset={resetDetectionParams}
          />
        ) : (
          <Alert severity="info">Loading contour detection defaults from Settings.</Alert>
        )}

        <Grid container spacing={2}>
          <Grid item xs={12} lg={8}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack direction="row" alignItems="center" justifyContent="space-between" gap={1}>
                    <Stack direction="row" alignItems="center" spacing={1}>
                      <ImageSearchIcon color="primary" />
                      <Typography variant="h6">Source And Candidate Overlay</Typography>
                    </Stack>
                    <Stack direction="row" spacing={1}>
                      <Button
                        variant="outlined"
                        startIcon={<RestartAltIcon />}
                        disabled={candidates.length === 0}
                        onClick={clearCandidates}
                      >
                        Clear
                      </Button>
                      <Button
                        variant="contained"
                        startIcon={
                          candidatesMutation.isPending ? (
                            <CircularProgress color="inherit" size={16} />
                          ) : (
                            <ImageSearchIcon />
                          )
                        }
                        disabled={
                          !effectiveResultId || !detectionParams || candidatesMutation.isPending
                        }
                        onClick={() => candidatesMutation.mutate()}
                      >
                        Detect
                      </Button>
                    </Stack>
                  </Stack>

                  {selectedResult ? (
                    <CandidateOverlay
                      result={selectedResult}
                      candidates={candidates}
                      selectedCandidateId={selectedCandidate?.candidateId ?? ''}
                      onSelect={selectCandidate}
                    />
                  ) : (
                    <Typography color="text.secondary">Open or upload an image first.</Typography>
                  )}

                  {candidatesMutation.data && (
                    <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                      <Chip
                        label={`${candidates.length} candidates`}
                        size="small"
                        color="primary"
                      />
                      <Chip
                        label={`${candidatesMutation.data.method.contourCount ?? 0} contours scanned`}
                        size="small"
                      />
                      <Chip
                        label={`retrieval ${candidatesMutation.data.method.retrieval}`}
                        size="small"
                      />
                    </Stack>
                  )}
                </Stack>
              </CardContent>
            </Card>
          </Grid>

          <Grid item xs={12} lg={4}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack direction="row" alignItems="center" spacing={1}>
                    <CenterFocusStrongIcon color="primary" />
                    <Typography variant="h6">Selection</Typography>
                  </Stack>

                  <CandidateList
                    candidates={candidates}
                    selectedCandidateId={selectedCandidate?.candidateId ?? ''}
                    onSelect={selectCandidate}
                    onClear={clearCandidates}
                  />

                  <Autocomplete
                    multiple
                    freeSolo
                    size="small"
                    options={
                      ocrLanguagesQuery.data?.availableLanguages.map((item) => item.code) ?? []
                    }
                    value={ocrLanguages}
                    onChange={(_event, value) => {
                      setOcrLanguagesCustomized(true);
                      setOcrLanguages(
                        value
                          .map((item) => item.trim())
                          .filter((item, index, items) => item && items.indexOf(item) === index),
                      );
                      setLatestOcr(null);
                    }}
                    renderInput={(params) => <TextField {...params} label="OCR Languages" />}
                  />

                  <TextField
                    select
                    label="Page Segmentation"
                    size="small"
                    fullWidth
                    value={ocrPageSegMode}
                    onChange={(event) => {
                      setOcrPageSegMode(event.target.value);
                      setLatestOcr(null);
                    }}
                  >
                    {OCR_PAGE_SEG_MODES.map((mode) => (
                      <MenuItem key={mode.value} value={mode.value}>
                        {mode.label}
                      </MenuItem>
                    ))}
                  </TextField>

                  {ocrLanguagesQuery.data && !ocrLanguagesQuery.data.tessdataAvailable && (
                    <Alert severity="info">
                      Add Tesseract traineddata files under models/tessdata or set TESSDATA_PREFIX
                      before running OCR.
                    </Alert>
                  )}

                  {selectedCandidate && (
                    <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                      <Chip
                        label={`Area ${formatPercent(selectedCandidate.areaRatio)}`}
                        size="small"
                      />
                      <Chip label={`Score ${selectedCandidate.score.toFixed(1)}`} size="small" />
                      <Chip
                        label={`${selectedCandidate.boundingBox.width} x ${selectedCandidate.boundingBox.height}`}
                        size="small"
                      />
                    </Stack>
                  )}

                  <Button
                    variant="contained"
                    color="success"
                    startIcon={<CenterFocusStrongIcon />}
                    disabled={!selectedCandidate || extractMutation.isPending}
                    onClick={() => extractMutation.mutate()}
                  >
                    Extract
                  </Button>
                  <Button
                    variant="outlined"
                    startIcon={
                      ocrMutation.isPending ? (
                        <CircularProgress color="inherit" size={16} />
                      ) : (
                        <TextFieldsIcon />
                      )
                    }
                    disabled={
                      !selectedCandidate || ocrLanguages.length === 0 || ocrMutation.isPending
                    }
                    onClick={() => ocrMutation.mutate()}
                  >
                    OCR
                  </Button>
                  <Box>
                    <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 1 }}>
                      <TextFieldsIcon color="primary" fontSize="small" />
                      <Typography variant="subtitle2">OCR Result</Typography>
                    </Stack>
                    <OcrResultPanel result={latestOcr} />
                  </Box>
                </Stack>
              </CardContent>
            </Card>
          </Grid>
        </Grid>

        <Grid container spacing={2}>
          <Grid item xs={12} lg={5}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Typography variant="h6">Candidate Preview</Typography>
                  {previewUrl ? (
                    <Box
                      component="img"
                      src={previewUrl}
                      alt="Selected contour preview"
                      sx={{
                        bgcolor: 'background.default',
                        border: 1,
                        borderColor: 'divider',
                        borderRadius: 1,
                        maxHeight: 420,
                        objectFit: 'contain',
                        width: '100%',
                      }}
                    />
                  ) : (
                    <Typography color="text.secondary">
                      Detect candidates and click a contour to preview its front-facing extraction.
                    </Typography>
                  )}
                  {latestExtracted && (
                    <Button
                      variant="contained"
                      startIcon={<SaveIcon />}
                      disabled={saveExtractionMutation.isPending}
                      onClick={() => saveExtractionMutation.mutate(latestExtracted.resultId)}
                    >
                      Save Latest Extracted PNG
                    </Button>
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
                    <Typography variant="h6">Extraction History</Typography>
                    {latestExtracted && (
                      <Chip
                        color="success"
                        size="small"
                        label={`Latest ${latestExtracted.width} x ${latestExtracted.height}`}
                      />
                    )}
                  </Stack>
                  <ExtractionHistory
                    results={extractionResults}
                    selectedId={latestExtracted?.resultId ?? ''}
                    onSelect={setLatestExtracted}
                    onDelete={(resultId) => deleteExtractionMutation.mutate(resultId)}
                    onSave={(resultId) => saveExtractionMutation.mutate(resultId)}
                    savingResultId={
                      saveExtractionMutation.isPending
                        ? (saveExtractionMutation.variables ?? '')
                        : ''
                    }
                  />
                </Stack>
              </CardContent>
            </Card>
          </Grid>
        </Grid>
      </Stack>
    </PlaceholderPage>
  );
}
