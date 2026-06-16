import {
  Alert,
  Button,
  Card,
  CardContent,
  Grid,
  LinearProgress,
  MenuItem,
  Stack,
  TextField,
  Typography,
} from '@mui/material';
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import { useEffect, useState } from 'react';
import {
  getSettings,
  updateSettings,
  type ContourDetectionSettings,
  type LoggingSettings,
  type PerformanceBenchmarkSettings,
} from '../../api/settingsApi';
import { PlaceholderPage } from '../../shared/components/PlaceholderPage';
import { useThemeMode, type ThemeMode } from '../../theme';

const themeOptions: Array<{ value: ThemeMode; label: string; helper: string }> = [
  { value: 'light', label: 'Light', helper: 'Use the light theme at all times.' },
  { value: 'dark', label: 'Dark', helper: 'Use the dark theme at all times.' },
  { value: 'system', label: 'System', helper: 'Follow the operating system preference.' },
];

function clampNumber(value: number, min: number, max: number) {
  return Math.max(min, Math.min(max, value));
}

export function SettingsPage() {
  const queryClient = useQueryClient();
  const { mode, resolvedMode, setMode } = useThemeMode();
  const [draftContour, setDraftContour] = useState<ContourDetectionSettings | null>(null);
  const [draftBenchmark, setDraftBenchmark] = useState<PerformanceBenchmarkSettings | null>(null);
  const [draftLogging, setDraftLogging] = useState<LoggingSettings | null>(null);

  const settingsQuery = useQuery({
    queryKey: ['settings'],
    queryFn: getSettings,
  });

  useEffect(() => {
    const settings = settingsQuery.data;
    if (!settings) {
      return;
    }
    setDraftContour(settings.opencv.contourDetection);
    setDraftBenchmark(settings.opencv.performanceBenchmark);
    setDraftLogging(settings.logging);
    if (settings.themeMode !== mode) {
      setMode(settings.themeMode);
    }
  }, [mode, setMode, settingsQuery.data]);

  const updateMutation = useMutation({
    mutationFn: updateSettings,
    onSuccess: (settings) => {
      setDraftContour(settings.opencv.contourDetection);
      setDraftBenchmark(settings.opencv.performanceBenchmark);
      setDraftLogging(settings.logging);
      void queryClient.invalidateQueries({ queryKey: ['settings'] });
    },
  });

  const handleThemeChange = (nextMode: ThemeMode) => {
    setMode(nextMode);
    updateMutation.mutate({ themeMode: nextMode });
  };

  const updateContourDraft = <Key extends keyof ContourDetectionSettings>(
    key: Key,
    value: ContourDetectionSettings[Key],
  ) => {
    setDraftContour((current) => (current ? { ...current, [key]: value } : current));
  };

  const updateBenchmarkDraft = <Key extends keyof PerformanceBenchmarkSettings>(
    key: Key,
    value: PerformanceBenchmarkSettings[Key],
  ) => {
    setDraftBenchmark((current) => {
      if (!current) {
        return current;
      }
      const next = { ...current, [key]: value };
      if (key === 'defaultIterations') {
        next.maxIterations = Math.max(next.maxIterations, next.defaultIterations);
      }
      if (key === 'maxIterations') {
        next.defaultIterations = Math.min(next.defaultIterations, next.maxIterations);
      }
      return next;
    });
  };

  const updateLoggingDraft = <Key extends keyof LoggingSettings>(
    key: Key,
    value: LoggingSettings[Key],
  ) => {
    setDraftLogging((current) => (current ? { ...current, [key]: value } : current));
  };

  const saveContourDefaults = () => {
    if (!draftContour) {
      return;
    }
    updateMutation.mutate({
      opencv: {
        contourDetection: draftContour,
      },
    });
  };

  const saveBenchmarkDefaults = () => {
    if (!draftBenchmark) {
      return;
    }
    updateMutation.mutate({
      opencv: {
        performanceBenchmark: draftBenchmark,
      },
    });
  };

  const saveLoggingSettings = () => {
    if (!draftLogging) {
      return;
    }
    updateMutation.mutate({
      logging: draftLogging,
    });
  };

  const error = settingsQuery.error ?? updateMutation.error;

  return (
    <PlaceholderPage
      title="Settings"
      eyebrow="Preferences"
      status={`Resolved ${resolvedMode}`}
      description="Application preferences that should remain consistent between desktop and LAN clients."
    >
      <Stack spacing={2.5}>
        {error instanceof Error && <Alert severity="warning">{error.message}</Alert>}
        {(settingsQuery.isLoading || updateMutation.isPending) && <LinearProgress />}
        {updateMutation.isSuccess && <Alert severity="success">Settings saved.</Alert>}

        <Card>
          <CardContent>
            <Stack spacing={2}>
              <Typography variant="h6">Theme Mode</Typography>
              <Grid container spacing={1.5}>
                {themeOptions.map((option) => (
                  <Grid item xs={12} md={4} key={option.value}>
                    <Button
                      fullWidth
                      variant={mode === option.value ? 'contained' : 'outlined'}
                      onClick={() => handleThemeChange(option.value)}
                      sx={{ alignItems: 'flex-start', justifyContent: 'flex-start', minHeight: 78 }}
                    >
                      <Stack alignItems="flex-start" spacing={0.25}>
                        <Typography>{option.label}</Typography>
                        <Typography variant="caption" sx={{ textAlign: 'left' }}>
                          {option.helper}
                        </Typography>
                      </Stack>
                    </Button>
                  </Grid>
                ))}
              </Grid>
            </Stack>
          </CardContent>
        </Card>

        <Card>
          <CardContent>
            <Stack spacing={2}>
              <Stack spacing={0.5}>
                <Typography variant="h6">OpenCV Contour Defaults</Typography>
                <Typography variant="body2" color="text.secondary">
                  These values initialize Contour Extractor. They are persisted by the backend in
                  data/settings.json.
                </Typography>
              </Stack>

              {draftContour ? (
                <>
                  <Grid container spacing={1.5}>
                    <Grid item xs={12} sm={6} md={2}>
                      <TextField
                        label="Max Candidates"
                        size="small"
                        type="number"
                        fullWidth
                        value={draftContour.maxCandidates}
                        inputProps={{ min: 1, max: 80 }}
                        onChange={(event) =>
                          updateContourDraft(
                            'maxCandidates',
                            clampNumber(Number(event.target.value) || 1, 1, 80),
                          )
                        }
                      />
                    </Grid>
                    <Grid item xs={12} sm={6} md={2}>
                      <TextField
                        label="Canny Low"
                        size="small"
                        type="number"
                        fullWidth
                        value={draftContour.low}
                        inputProps={{ min: 1, max: 255 }}
                        onChange={(event) =>
                          updateContourDraft(
                            'low',
                            clampNumber(Number(event.target.value) || 1, 1, 255),
                          )
                        }
                      />
                    </Grid>
                    <Grid item xs={12} sm={6} md={2}>
                      <TextField
                        label="Canny High"
                        size="small"
                        type="number"
                        fullWidth
                        value={draftContour.high}
                        inputProps={{ min: 1, max: 255 }}
                        onChange={(event) =>
                          updateContourDraft(
                            'high',
                            clampNumber(
                              Number(event.target.value) || draftContour.low + 1,
                              draftContour.low + 1,
                              255,
                            ),
                          )
                        }
                      />
                    </Grid>
                    <Grid item xs={12} sm={6} md={2}>
                      <TextField
                        label="Min Area %"
                        size="small"
                        type="number"
                        fullWidth
                        value={(draftContour.minAreaRatio * 100).toFixed(2)}
                        inputProps={{ min: 0.01, max: 50, step: 0.01 }}
                        onChange={(event) =>
                          updateContourDraft(
                            'minAreaRatio',
                            clampNumber((Number(event.target.value) || 0.01) / 100, 0.0001, 0.5),
                          )
                        }
                      />
                    </Grid>
                    <Grid item xs={12} sm={6} md={2}>
                      <TextField
                        label="Epsilon"
                        size="small"
                        type="number"
                        fullWidth
                        value={draftContour.epsilon}
                        inputProps={{ min: 0.005, max: 0.12, step: 0.001 }}
                        onChange={(event) =>
                          updateContourDraft(
                            'epsilon',
                            clampNumber(Number(event.target.value) || 0.005, 0.005, 0.12),
                          )
                        }
                      />
                    </Grid>
                    <Grid item xs={12} sm={6} md={2}>
                      <TextField
                        select
                        label="Retrieval"
                        size="small"
                        fullWidth
                        value={draftContour.retrieval}
                        onChange={(event) =>
                          updateContourDraft('retrieval', event.target.value as 'list' | 'external')
                        }
                      >
                        <MenuItem value="list">List</MenuItem>
                        <MenuItem value="external">External</MenuItem>
                      </TextField>
                    </Grid>
                  </Grid>
                  <Stack direction="row" justifyContent="flex-end">
                    <Button
                      variant="contained"
                      disabled={updateMutation.isPending}
                      onClick={saveContourDefaults}
                    >
                      Save OpenCV Defaults
                    </Button>
                  </Stack>
                </>
              ) : (
                <Typography color="text.secondary">Loading OpenCV contour defaults.</Typography>
              )}
            </Stack>
          </CardContent>
        </Card>

        <Card>
          <CardContent>
            <Stack spacing={2}>
              <Stack spacing={0.5}>
                <Typography variant="h6">Performance Benchmark Defaults</Typography>
                <Typography variant="body2" color="text.secondary">
                  These values are used by Performance Lab and benchmark job assignment controls.
                </Typography>
              </Stack>

              {draftBenchmark ? (
                <>
                  <Grid container spacing={1.5}>
                    <Grid item xs={12} sm={6} md={3}>
                      <TextField
                        label="Default Iterations"
                        size="small"
                        type="number"
                        fullWidth
                        value={draftBenchmark.defaultIterations}
                        inputProps={{ min: 1, max: draftBenchmark.maxIterations }}
                        onChange={(event) =>
                          updateBenchmarkDraft(
                            'defaultIterations',
                            clampNumber(
                              Number(event.target.value) || 1,
                              1,
                              draftBenchmark.maxIterations,
                            ),
                          )
                        }
                      />
                    </Grid>
                    <Grid item xs={12} sm={6} md={3}>
                      <TextField
                        label="Max Iterations"
                        size="small"
                        type="number"
                        fullWidth
                        value={draftBenchmark.maxIterations}
                        inputProps={{ min: draftBenchmark.defaultIterations, max: 100 }}
                        onChange={(event) =>
                          updateBenchmarkDraft(
                            'maxIterations',
                            clampNumber(
                              Number(event.target.value) || draftBenchmark.defaultIterations,
                              draftBenchmark.defaultIterations,
                              100,
                            ),
                          )
                        }
                      />
                    </Grid>
                  </Grid>
                  <Stack direction="row" justifyContent="flex-end">
                    <Button
                      variant="contained"
                      disabled={updateMutation.isPending}
                      onClick={saveBenchmarkDefaults}
                    >
                      Save Benchmark Defaults
                    </Button>
                  </Stack>
                </>
              ) : (
                <Typography color="text.secondary">Loading benchmark defaults.</Typography>
              )}
            </Stack>
          </CardContent>
        </Card>

        <Card>
          <CardContent>
            <Stack spacing={2}>
              <Stack spacing={0.5}>
                <Typography variant="h6">Rolling Logs</Typography>
                <Typography variant="body2" color="text.secondary">
                  Backend logs are written under logs/backend.log and rotated by size.
                </Typography>
              </Stack>

              {draftLogging ? (
                <>
                  <Grid container spacing={1.5}>
                    <Grid item xs={12} sm={6} md={3}>
                      <TextField
                        select
                        label="Log Level"
                        size="small"
                        fullWidth
                        value={draftLogging.level}
                        onChange={(event) =>
                          updateLoggingDraft(
                            'level',
                            event.target.value as LoggingSettings['level'],
                          )
                        }
                      >
                        <MenuItem value="debug">Debug</MenuItem>
                        <MenuItem value="info">Info</MenuItem>
                        <MenuItem value="warning">Warning</MenuItem>
                        <MenuItem value="error">Error</MenuItem>
                      </TextField>
                    </Grid>
                    <Grid item xs={12} sm={6} md={3}>
                      <TextField
                        label="Max File MB"
                        size="small"
                        type="number"
                        fullWidth
                        value={draftLogging.maxFileMb}
                        inputProps={{ min: 1, max: 100 }}
                        onChange={(event) =>
                          updateLoggingDraft(
                            'maxFileMb',
                            clampNumber(Number(event.target.value) || 1, 1, 100),
                          )
                        }
                      />
                    </Grid>
                    <Grid item xs={12} sm={6} md={3}>
                      <TextField
                        label="Retention Days"
                        size="small"
                        type="number"
                        fullWidth
                        value={draftLogging.retentionDays}
                        inputProps={{ min: 1, max: 30 }}
                        onChange={(event) =>
                          updateLoggingDraft(
                            'retentionDays',
                            clampNumber(Number(event.target.value) || 1, 1, 30),
                          )
                        }
                      />
                    </Grid>
                  </Grid>
                  <Stack direction="row" justifyContent="flex-end">
                    <Button
                      variant="contained"
                      disabled={updateMutation.isPending}
                      onClick={saveLoggingSettings}
                    >
                      Save Log Settings
                    </Button>
                  </Stack>
                </>
              ) : (
                <Typography color="text.secondary">Loading rolling log settings.</Typography>
              )}
            </Stack>
          </CardContent>
        </Card>
      </Stack>
    </PlaceholderPage>
  );
}
