import AccountTreeIcon from '@mui/icons-material/AccountTree';
import AssignmentTurnedInIcon from '@mui/icons-material/AssignmentTurnedIn';
import BarChartIcon from '@mui/icons-material/BarChart';
import DonutLargeIcon from '@mui/icons-material/DonutLarge';
import ImageIcon from '@mui/icons-material/Image';
import MovieFilterIcon from '@mui/icons-material/MovieFilter';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import TimelineIcon from '@mui/icons-material/Timeline';
import { Alert, Box, Button, Card, CardContent, Chip, Divider, Grid, MenuItem, Stack, TextField, Typography } from '@mui/material';
import { alpha } from '@mui/material/styles';
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import { useState } from 'react';
import { getImageResults, processImage, type ImageOperation } from '../../api/imageApi';
import { getJobs, type JobRecord } from '../../api/jobsApi';
import { executePipeline, getPipelines } from '../../api/pipelineApi';
import { exportVideo, getVideoDiagnosticsHistory, getVideos, processVideo, type VideoFilter } from '../../api/videoApi';
import { PlaceholderPage } from '../../shared/components/PlaceholderPage';

type ChartDatum = {
  label: string;
  value: number;
};

type JobTemplate = 'image' | 'video-preview' | 'video-export' | 'pipeline';

const imageOperationOptions: Array<{ value: ImageOperation; label: string }> = [
  { value: 'grayscale', label: 'Grayscale' },
  { value: 'blur', label: 'Blur' },
  { value: 'threshold', label: 'Threshold' },
  { value: 'edgeDetect', label: 'Edge Detect' },
  { value: 'histogram', label: 'Histogram' },
];

const videoFilterOptions: Array<{ value: VideoFilter; label: string }> = [
  { value: 'none', label: 'Original' },
  { value: 'grayscale', label: 'Grayscale' },
  { value: 'blur', label: 'Blur' },
  { value: 'edgeDetect', label: 'Edge Detect' },
  { value: 'threshold', label: 'Threshold' },
  { value: 'opticalFlow', label: 'Optical Flow' },
  { value: 'stabilize', label: 'Stabilize' },
];

function defaultImageParams(operation: ImageOperation): Record<string, unknown> {
  if (operation === 'blur') {
    return { kernel: 7 };
  }
  if (operation === 'threshold') {
    return { threshold: 128 };
  }
  if (operation === 'edgeDetect') {
    return { low: 80, high: 160 };
  }
  return {};
}

function countBy<T>(items: T[], getKey: (item: T) => string) {
  return items.reduce<Record<string, number>>((counts, item) => {
    const key = getKey(item);
    counts[key] = (counts[key] ?? 0) + 1;
    return counts;
  }, {});
}

function toData(counts: Record<string, number>) {
  return Object.entries(counts)
    .map(([label, value]) => ({ label, value }))
    .sort((left, right) => right.value - left.value);
}

function formatTime(value: string) {
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) {
    return value;
  }
  return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
}

function BarList({ data, emptyLabel }: { data: ChartDatum[]; emptyLabel: string }) {
  const max = Math.max(...data.map((item) => item.value), 1);

  return (
    <Stack spacing={1.25}>
      {data.map((item) => (
        <Stack key={item.label} spacing={0.5}>
          <Stack direction="row" justifyContent="space-between" spacing={1}>
            <Typography variant="body2">{item.label}</Typography>
            <Typography variant="body2" color="text.secondary">
              {item.value}
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
                width: `${Math.max(6, (item.value / max) * 100)}%`,
              }}
            />
          </Box>
        </Stack>
      ))}
      {data.length === 0 && <Typography color="text.secondary">{emptyLabel}</Typography>}
    </Stack>
  );
}

function TimelineBars({ jobs }: { jobs: JobRecord[] }) {
  const rows = jobs
    .slice()
    .sort((left, right) => new Date(left.updatedAt).getTime() - new Date(right.updatedAt).getTime())
    .slice(-10);

  return (
    <Stack direction="row" spacing={1} alignItems="flex-end" sx={{ minHeight: 160, overflowX: 'auto', py: 1 }}>
      {rows.map((job) => (
        <Stack key={job.id} spacing={1} alignItems="center" sx={{ minWidth: 54 }}>
          <Box
            title={`${job.type} ${job.progress}%`}
            sx={{
              bgcolor: job.status === 'failed' ? 'error.main' : job.status === 'completed' ? 'success.main' : 'warning.main',
              borderRadius: 1,
              height: `${Math.max(12, job.progress * 1.3)}px`,
              width: 24,
            }}
          />
          <Typography variant="caption" color="text.secondary">
            {formatTime(job.updatedAt)}
          </Typography>
        </Stack>
      ))}
      {rows.length === 0 && <Typography color="text.secondary">No job timeline data is available.</Typography>}
    </Stack>
  );
}

export function ChartShowcasePage() {
  const queryClient = useQueryClient();
  const [jobTemplate, setJobTemplate] = useState<JobTemplate>('image');
  const [selectedImageId, setSelectedImageId] = useState('');
  const [selectedVideoId, setSelectedVideoId] = useState('');
  const [selectedPipelineId, setSelectedPipelineId] = useState('');
  const [imageOperation, setImageOperation] = useState<ImageOperation>('grayscale');
  const [videoFilter, setVideoFilter] = useState<VideoFilter>('grayscale');
  const [assignedJob, setAssignedJob] = useState<JobRecord | null>(null);

  const jobsQuery = useQuery({ queryKey: ['jobs'], queryFn: getJobs, refetchInterval: 5000 });
  const imageResultsQuery = useQuery({ queryKey: ['image-results'], queryFn: getImageResults, refetchInterval: 10000 });
  const pipelinesQuery = useQuery({ queryKey: ['pipelines'], queryFn: getPipelines, refetchInterval: 10000 });
  const videosQuery = useQuery({ queryKey: ['video-library'], queryFn: getVideos, refetchInterval: 10000 });
  const diagnosticsQuery = useQuery({ queryKey: ['video-diagnostics'], queryFn: getVideoDiagnosticsHistory, refetchInterval: 10000 });

  const jobs = jobsQuery.data?.jobs ?? [];
  const results = imageResultsQuery.data?.results ?? [];
  const executions = pipelinesQuery.data?.executions ?? [];
  const videos = videosQuery.data?.videos ?? [];
  const diagnostics = diagnosticsQuery.data?.records ?? [];
  const pipelines = pipelinesQuery.data?.pipelines ?? [];
  const statusData = toData(countBy(jobs, (job) => job.status));
  const operationData = toData(countBy(results, (result) => result.operation));
  const pipelineStatusData = toData(countBy(executions, (execution) => execution.status));
  const videoReadFpsData = diagnostics.slice(0, 8).map((diagnostic) => ({
    label: `${diagnostic.videoName} (${formatTime(diagnostic.createdAt)})`,
    value: Number(diagnostic.measuredReadFps.toFixed(1)),
  }));
  const motionMetricData = diagnostics
    .filter((diagnostic) => diagnostic.trackedFeatures !== undefined || diagnostic.averageFlowMagnitude !== undefined)
    .slice(0, 8)
    .map((diagnostic) => ({
      label: `${diagnostic.operation ?? 'motion'} ${diagnostic.videoName}`,
      value: Number((diagnostic.averageFlowMagnitude ?? diagnostic.trackedFeatures ?? 0).toFixed(1)),
    }));
  const effectiveImageId = selectedImageId || results[0]?.resultId || '';
  const effectiveVideoId = selectedVideoId || videos[0]?.videoId || '';
  const effectivePipelineId = selectedPipelineId || pipelines[0]?.id || '';
  const selectedVideo = videos.find((video) => video.videoId === effectiveVideoId);
  const selectedPipeline = pipelines.find((pipeline) => pipeline.id === effectivePipelineId);

  const assignJobMutation = useMutation({
    mutationFn: async () => {
      if (jobTemplate === 'image') {
        const response = await processImage({
          resultId: effectiveImageId,
          operation: imageOperation,
          params: defaultImageParams(imageOperation),
        });
        return response.job;
      }

      if (jobTemplate === 'video-preview') {
        const response = await processVideo({
          videoId: effectiveVideoId,
          filter: videoFilter,
          startFrame: 0,
          endFrame: 0,
        });
        return response.job;
      }

      if (jobTemplate === 'video-export') {
        const response = await exportVideo({
          videoId: effectiveVideoId,
          filter: videoFilter,
          startFrame: 0,
          endFrame: Math.max(0, (selectedVideo?.frameCount ?? 1) - 1),
        });
        return response.job;
      }

      if (!selectedPipeline) {
        throw new Error('Select a saved pipeline before assigning the job.');
      }

      const response = await executePipeline(selectedPipeline.document);
      return response.execution.job;
    },
    onSuccess: (job) => {
      setAssignedJob(job);
      void queryClient.invalidateQueries({ queryKey: ['jobs'] });
      void queryClient.invalidateQueries({ queryKey: ['image-results'] });
      void queryClient.invalidateQueries({ queryKey: ['pipelines'] });
    },
  });

  const canAssignJob =
    !assignJobMutation.isPending &&
    ((jobTemplate === 'image' && Boolean(effectiveImageId)) ||
      ((jobTemplate === 'video-preview' || jobTemplate === 'video-export') && Boolean(effectiveVideoId)) ||
      (jobTemplate === 'pipeline' && Boolean(selectedPipeline)));

  return (
    <PlaceholderPage
      title="Chart Showcase"
      eyebrow="Analytics"
      status={`${jobs.length} jobs`}
      description="Processing statistics are derived from server-owned job records, image results, pipeline executions, and video diagnostics."
    >
      <Stack spacing={2.5}>
        {(jobsQuery.isError || imageResultsQuery.isError || pipelinesQuery.isError || videosQuery.isError || diagnosticsQuery.isError) && (
          <Alert severity="warning">Some chart data is not available from the backend.</Alert>
        )}
        {assignJobMutation.isError && (
          <Alert severity="error">
            {assignJobMutation.error instanceof Error ? assignJobMutation.error.message : 'Unable to assign the selected job.'}
          </Alert>
        )}

        <Card>
          <CardContent>
            <Stack spacing={2}>
              <Stack direction="row" justifyContent="space-between" alignItems="center" flexWrap="wrap" gap={1}>
                <Stack direction="row" spacing={1} alignItems="center">
                  <AssignmentTurnedInIcon color="primary" />
                  <Typography variant="h6">Job Assignment</Typography>
                </Stack>
                {assignedJob && (
                  <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                    <Chip label={assignedJob.type} size="small" color="primary" />
                    <Chip label={assignedJob.status} size="small" variant="outlined" />
                    <Chip label={`${assignedJob.progress}%`} size="small" variant="outlined" />
                  </Stack>
                )}
              </Stack>

              <Alert severity="info">
                Select an operation source, assign it as a backend job, then watch the status and timeline charts refresh from
                server-owned job records.
              </Alert>

              <Grid container spacing={2}>
                <Grid item xs={12} md={3}>
                  <TextField
                    select
                    label="Job Type"
                    size="small"
                    fullWidth
                    value={jobTemplate}
                    onChange={(event) => setJobTemplate(event.target.value as JobTemplate)}
                  >
                    <MenuItem value="image">
                      <Stack direction="row" spacing={1} alignItems="center">
                        <ImageIcon fontSize="small" />
                        <span>Image operation</span>
                      </Stack>
                    </MenuItem>
                    <MenuItem value="video-preview">
                      <Stack direction="row" spacing={1} alignItems="center">
                        <MovieFilterIcon fontSize="small" />
                        <span>Video preview</span>
                      </Stack>
                    </MenuItem>
                    <MenuItem value="video-export">
                      <Stack direction="row" spacing={1} alignItems="center">
                        <MovieFilterIcon fontSize="small" />
                        <span>Video export</span>
                      </Stack>
                    </MenuItem>
                    <MenuItem value="pipeline">
                      <Stack direction="row" spacing={1} alignItems="center">
                        <AccountTreeIcon fontSize="small" />
                        <span>Pipeline run</span>
                      </Stack>
                    </MenuItem>
                  </TextField>
                </Grid>

                {jobTemplate === 'image' && (
                  <>
                    <Grid item xs={12} md={5}>
                      <TextField
                        select
                        label="Image Result"
                        size="small"
                        fullWidth
                        value={effectiveImageId}
                        onChange={(event) => setSelectedImageId(event.target.value)}
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
                    <Grid item xs={12} md={2}>
                      <TextField
                        select
                        label="Operation"
                        size="small"
                        fullWidth
                        value={imageOperation}
                        onChange={(event) => setImageOperation(event.target.value as ImageOperation)}
                      >
                        {imageOperationOptions.map((option) => (
                          <MenuItem key={option.value} value={option.value}>
                            {option.label}
                          </MenuItem>
                        ))}
                      </TextField>
                    </Grid>
                  </>
                )}

                {(jobTemplate === 'video-preview' || jobTemplate === 'video-export') && (
                  <>
                    <Grid item xs={12} md={5}>
                      <TextField
                        select
                        label="Video"
                        size="small"
                        fullWidth
                        value={effectiveVideoId}
                        onChange={(event) => setSelectedVideoId(event.target.value)}
                      >
                        <MenuItem value="" disabled>
                          No video selected
                        </MenuItem>
                        {videos.map((video) => (
                          <MenuItem key={video.videoId} value={video.videoId}>
                            {video.name} - {video.width}x{video.height}
                          </MenuItem>
                        ))}
                      </TextField>
                    </Grid>
                    <Grid item xs={12} md={2}>
                      <TextField
                        select
                        label="Filter"
                        size="small"
                        fullWidth
                        value={videoFilter}
                        onChange={(event) => setVideoFilter(event.target.value as VideoFilter)}
                      >
                        {videoFilterOptions.map((option) => (
                          <MenuItem key={option.value} value={option.value}>
                            {option.label}
                          </MenuItem>
                        ))}
                      </TextField>
                    </Grid>
                  </>
                )}

                {jobTemplate === 'pipeline' && (
                  <Grid item xs={12} md={7}>
                    <TextField
                      select
                      label="Saved Pipeline"
                      size="small"
                      fullWidth
                      value={effectivePipelineId}
                      onChange={(event) => setSelectedPipelineId(event.target.value)}
                    >
                      <MenuItem value="" disabled>
                        No saved pipeline selected
                      </MenuItem>
                      {pipelines.map((pipeline) => (
                        <MenuItem key={pipeline.id} value={pipeline.id}>
                          {pipeline.document.name}
                        </MenuItem>
                      ))}
                    </TextField>
                  </Grid>
                )}

                <Grid item xs={12} md={2}>
                  <Button
                    fullWidth
                    variant="contained"
                    startIcon={<PlayArrowIcon />}
                    onClick={() => assignJobMutation.mutate()}
                    disabled={!canAssignJob}
                    sx={{ height: '100%' }}
                  >
                    Assign
                  </Button>
                </Grid>
              </Grid>

              <Divider />
              <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                <Chip label="1. Select source" size="small" />
                <Chip label="2. Assign job" size="small" />
                <Chip label="3. Watch progress" size="small" />
              </Stack>
            </Stack>
          </CardContent>
        </Card>

        <Grid container spacing={2}>
          <Grid item xs={12} md={3}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack direction="row" spacing={1} alignItems="center">
                    <DonutLargeIcon color="primary" />
                    <Typography variant="h6">Job Status</Typography>
                  </Stack>
                  <BarList data={statusData} emptyLabel="No jobs have been recorded." />
                </Stack>
              </CardContent>
            </Card>
          </Grid>

          <Grid item xs={12} md={3}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack direction="row" spacing={1} alignItems="center">
                    <BarChartIcon color="primary" />
                    <Typography variant="h6">Image Operations</Typography>
                  </Stack>
                  <BarList data={operationData} emptyLabel="No image results are available." />
                </Stack>
              </CardContent>
            </Card>
          </Grid>

          <Grid item xs={12} md={3}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack direction="row" spacing={1} alignItems="center">
                    <TimelineIcon color="primary" />
                    <Typography variant="h6">Pipeline Runs</Typography>
                  </Stack>
                  <BarList data={pipelineStatusData} emptyLabel="No pipeline executions are available." />
                </Stack>
              </CardContent>
            </Card>
          </Grid>

          <Grid item xs={12} md={3}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack direction="row" spacing={1} alignItems="center">
                    <MovieFilterIcon color="primary" />
                    <Typography variant="h6">Video Diagnostics</Typography>
                  </Stack>
                  <BarList data={videoReadFpsData} emptyLabel="Run Measure FPS in Video Lab to collect diagnostics." />
                </Stack>
              </CardContent>
            </Card>
          </Grid>
          <Grid item xs={12} md={3}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack direction="row" spacing={1} alignItems="center">
                    <MovieFilterIcon color="primary" />
                    <Typography variant="h6">Motion Metrics</Typography>
                  </Stack>
                  <BarList data={motionMetricData} emptyLabel="Run Analyze Motion in Video Lab to collect flow metrics." />
                </Stack>
              </CardContent>
            </Card>
          </Grid>
        </Grid>

        <Card>
          <CardContent>
            <Stack spacing={2}>
              <Stack direction="row" justifyContent="space-between" alignItems="center" flexWrap="wrap" gap={1}>
                <Stack direction="row" spacing={1} alignItems="center">
                  <TimelineIcon color="primary" />
                  <Typography variant="h6">Recent Job Progress</Typography>
                </Stack>
                <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                  <Chip label={`${jobs.length} jobs`} size="small" variant="outlined" />
                  <Chip label={`${results.length} image results`} size="small" variant="outlined" />
                  <Chip label={`${executions.length} pipeline executions`} size="small" variant="outlined" />
                  <Chip label={`${diagnostics.length} video diagnostics`} size="small" variant="outlined" />
                </Stack>
              </Stack>
              <TimelineBars jobs={jobs} />
            </Stack>
          </CardContent>
        </Card>
      </Stack>
    </PlaceholderPage>
  );
}
