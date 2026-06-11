import DownloadIcon from '@mui/icons-material/Download';
import FolderOpenIcon from '@mui/icons-material/FolderOpen';
import MovieFilterIcon from '@mui/icons-material/MovieFilter';
import PauseIcon from '@mui/icons-material/Pause';
import PhotoCameraIcon from '@mui/icons-material/PhotoCamera';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
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
  exportVideo,
  extractVideoFrame,
  getVideo,
  getVideos,
  processVideo,
  videoFrameUrl,
  type VideoExportResult,
  type VideoFilter,
  type VideoFrameResult,
  type VideoRecord,
} from '../../api/videoApi';
import { getVideoOpenCapabilities, openVideoFromLocalPath, openVideoFromUpload } from '../../runtime/fileAdapter';
import { getRuntimeMode, type RuntimeMode } from '../../runtime/runtimeMode';
import { PlaceholderPage } from '../../shared/components/PlaceholderPage';

const videosQueryKey = ['video-library'];

const filterOptions: Array<{ value: VideoFilter; label: string }> = [
  { value: 'none', label: 'Original' },
  { value: 'grayscale', label: 'Grayscale' },
  { value: 'blur', label: 'Blur' },
  { value: 'edgeDetect', label: 'Edge Detect' },
  { value: 'threshold', label: 'Threshold' },
];

function mutationErrorMessage(error: unknown) {
  return error instanceof Error ? error.message : 'Video operation failed.';
}

function formatDuration(seconds: number) {
  if (!Number.isFinite(seconds) || seconds <= 0) {
    return '0:00';
  }

  const minutes = Math.floor(seconds / 60);
  const remainingSeconds = Math.floor(seconds % 60)
    .toString()
    .padStart(2, '0');
  return `${minutes}:${remainingSeconds}`;
}

function clampFrame(value: number, video?: VideoRecord | null) {
  return Math.min(Math.max(0, Math.round(value)), Math.max(0, (video?.frameCount ?? 1) - 1));
}

function FramePreview({ src, label }: { src?: string; label: string }) {
  return (
    <Stack spacing={1}>
      <Typography variant="subtitle2">{label}</Typography>
      <Box
        sx={{
          alignItems: 'center',
          bgcolor: 'background.default',
          border: 1,
          borderColor: 'divider',
          borderRadius: 2,
          display: 'flex',
          justifyContent: 'center',
          minHeight: { xs: 220, md: 420 },
          overflow: 'hidden',
        }}
      >
        {src ? (
          <Box
            component="img"
            alt={`${label} video frame preview`}
            src={src}
            sx={{
              display: 'block',
              maxHeight: { xs: 300, md: 520 },
              maxWidth: '100%',
              objectFit: 'contain',
            }}
          />
        ) : (
          <Typography color="text.secondary">Open a video to inspect frames.</Typography>
        )}
      </Box>
    </Stack>
  );
}

export function VideoLabPage() {
  const queryClient = useQueryClient();
  const [runtimeMode, setRuntimeMode] = useState<RuntimeMode>('desktop');
  const [localPath, setLocalPath] = useState('');
  const [currentVideo, setCurrentVideo] = useState<VideoRecord | null>(null);
  const [frameIndex, setFrameIndex] = useState(0);
  const [filter, setFilter] = useState<VideoFilter>('none');
  const [cacheKey, setCacheKey] = useState(0);
  const [isPlaying, setIsPlaying] = useState(false);
  const [playbackFps, setPlaybackFps] = useState(12);
  const [frameResult, setFrameResult] = useState<VideoFrameResult | null>(null);
  const [exportResult, setExportResult] = useState<VideoExportResult | null>(null);

  useEffect(() => {
    setRuntimeMode(getRuntimeMode());
  }, []);

  const capabilities = useMemo(() => getVideoOpenCapabilities(runtimeMode), [runtimeMode]);

  const videoQuery = useQuery({
    queryKey: [...videosQueryKey, currentVideo?.videoId],
    queryFn: () => getVideo(currentVideo?.videoId ?? ''),
    enabled: Boolean(currentVideo?.videoId),
  });

  const videosQuery = useQuery({
    queryKey: videosQueryKey,
    queryFn: getVideos,
    refetchInterval: 8000,
  });

  const activeVideo = videoQuery.data ?? currentVideo;
  const maxFrame = Math.max(0, (activeVideo?.frameCount ?? 1) - 1);
  const previewSrc = activeVideo ? videoFrameUrl(activeVideo.videoId, frameIndex, filter, cacheKey) : undefined;
  const originalPreviewSrc = activeVideo ? videoFrameUrl(activeVideo.videoId, frameIndex, 'none', cacheKey) : undefined;

  useEffect(() => {
    if (!isPlaying || !activeVideo) {
      return undefined;
    }

    const interval = window.setInterval(() => {
      setFrameIndex((currentFrame) => {
        const nextFrame = currentFrame + 1;
        if (nextFrame > maxFrame) {
          setIsPlaying(false);
          return maxFrame;
        }
        return nextFrame;
      });
    }, Math.max(33, 1000 / playbackFps));

    return () => window.clearInterval(interval);
  }, [activeVideo, isPlaying, maxFrame, playbackFps]);

  const resetForVideo = (video: VideoRecord) => {
    setCurrentVideo(video);
    setFrameIndex(0);
    setIsPlaying(false);
    setFrameResult(null);
    setExportResult(null);
    setCacheKey(Date.now());
  };

  const openLocalMutation = useMutation({
    mutationFn: () => openVideoFromLocalPath(localPath, runtimeMode),
    onSuccess: (video) => {
      resetForVideo(video);
      void queryClient.invalidateQueries({ queryKey: videosQueryKey });
    },
  });

  const uploadMutation = useMutation({
    mutationFn: openVideoFromUpload,
    onSuccess: (video) => {
      resetForVideo(video);
      void queryClient.invalidateQueries({ queryKey: videosQueryKey });
    },
  });

  const processMutation = useMutation({
    mutationFn: () =>
      processVideo({
        videoId: activeVideo?.videoId ?? '',
        filter,
        startFrame: frameIndex,
        endFrame: frameIndex,
      }),
    onSuccess: () => {
      setCacheKey(Date.now());
      void queryClient.invalidateQueries({ queryKey: ['jobs'] });
    },
  });

  const extractMutation = useMutation({
    mutationFn: () =>
      extractVideoFrame({
        videoId: activeVideo?.videoId ?? '',
        frameIndex,
        filter,
      }),
    onSuccess: ({ result }) => {
      setFrameResult(result);
      setCacheKey(Date.now());
      void queryClient.invalidateQueries({ queryKey: ['jobs'] });
    },
  });

  const exportMutation = useMutation({
    mutationFn: () =>
      exportVideo({
        videoId: activeVideo?.videoId ?? '',
        filter,
        startFrame: 0,
        endFrame: maxFrame,
      }),
    onSuccess: ({ result }) => {
      setExportResult(result);
      void queryClient.invalidateQueries({ queryKey: ['jobs'] });
    },
  });

  const busy =
    openLocalMutation.isPending ||
    uploadMutation.isPending ||
    processMutation.isPending ||
    extractMutation.isPending ||
    exportMutation.isPending;
  const currentError =
    openLocalMutation.error ??
    uploadMutation.error ??
    processMutation.error ??
    extractMutation.error ??
    exportMutation.error;

  const changeFilter = (event: ChangeEvent<HTMLInputElement>) => {
    setFilter(event.target.value as VideoFilter);
    setCacheKey(Date.now());
  };

  const changeFrame = (_: Event, value: number | number[]) => {
    setFrameIndex(clampFrame(Array.isArray(value) ? value[0] : value, activeVideo));
    setIsPlaying(false);
    setFrameResult(null);
  };

  return (
    <PlaceholderPage
      title="Video Lab"
      eyebrow="Media"
      status={activeVideo ? `${activeVideo.width}x${activeVideo.height}` : 'Ready'}
      description="Inspect video metadata, extract frames, run frame filters, and export processed clips."
    >
      <Stack spacing={2.5}>
        {runtimeMode === 'lan' && (
          <Alert severity="info">LAN clients can upload videos and inspect results without arbitrary local path access.</Alert>
        )}
        {currentError && <Alert severity="error">{mutationErrorMessage(currentError)}</Alert>}
        {frameResult && <Alert severity="success">Frame saved to {frameResult.path}</Alert>}
        {exportResult && <Alert severity="success">Video exported to {exportResult.path}</Alert>}

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
                    label="Local Video Path"
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
                        accept="video/mp4,video/quicktime,video/x-msvideo,video/x-matroska,video/x-ms-wmv"
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

                  {activeVideo && (
                    <>
                      <Divider />
                      <Stack spacing={0.75}>
                        <Typography variant="subtitle2">{activeVideo.name}</Typography>
                        <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                          <Chip label={`${activeVideo.width}x${activeVideo.height}`} size="small" />
                          <Chip label={`${Math.round(activeVideo.fps * 100) / 100} fps`} size="small" />
                          <Chip label={`${activeVideo.frameCount} frames`} size="small" />
                          <Chip label={formatDuration(activeVideo.durationSeconds)} size="small" />
                          <Chip label={activeVideo.sourceType} size="small" />
                        </Stack>
                      </Stack>
                    </>
                  )}
                </Stack>
              </CardContent>
            </Card>
          </Grid>

          <Grid item xs={12} lg={4}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack direction="row" alignItems="center" justifyContent="space-between" gap={1}>
                    <Typography variant="h6">Frame Filter</Typography>
                    <MovieFilterIcon color="primary" />
                  </Stack>
                  <TextField select label="Filter" value={filter} onChange={changeFilter} fullWidth size="small">
                    {filterOptions.map((option) => (
                      <MenuItem key={option.value} value={option.value}>
                        {option.label}
                      </MenuItem>
                    ))}
                  </TextField>
                  <Stack spacing={0.75}>
                    <Stack direction="row" justifyContent="space-between" gap={1}>
                      <Typography variant="body2" color="text.secondary">
                        Frame
                      </Typography>
                      <Typography variant="body2" color="text.secondary">
                        {frameIndex} / {maxFrame}
                      </Typography>
                    </Stack>
                    <Slider
                      value={frameIndex}
                      min={0}
                      max={maxFrame}
                      step={1}
                      valueLabelDisplay="auto"
                      onChange={changeFrame}
                      disabled={!activeVideo || busy}
                    />
                  </Stack>
                  <Stack spacing={0.75}>
                    <Stack direction="row" justifyContent="space-between" gap={1}>
                      <Typography variant="body2" color="text.secondary">
                        Playback rate
                      </Typography>
                      <Typography variant="body2" color="text.secondary">
                        {playbackFps} fps
                      </Typography>
                    </Stack>
                    <Slider
                      value={playbackFps}
                      min={1}
                      max={30}
                      step={1}
                      valueLabelDisplay="auto"
                      onChange={(_, value) => setPlaybackFps(Array.isArray(value) ? value[0] : value)}
                      disabled={!activeVideo}
                    />
                  </Stack>

                  <Stack direction={{ xs: 'column', sm: 'row' }} spacing={1}>
                    <Button
                      startIcon={isPlaying ? <PauseIcon /> : <PlayArrowIcon />}
                      variant="contained"
                      onClick={() => setIsPlaying((playing) => !playing)}
                      disabled={!activeVideo}
                    >
                      {isPlaying ? 'Pause' : 'Play'}
                    </Button>
                    <Button
                      startIcon={<MovieFilterIcon />}
                      onClick={() => processMutation.mutate()}
                      disabled={!activeVideo || busy}
                    >
                      Preview
                    </Button>
                    <Button
                      variant="outlined"
                      startIcon={<PhotoCameraIcon />}
                      onClick={() => extractMutation.mutate()}
                      disabled={!activeVideo || busy}
                    >
                      Extract PNG
                    </Button>
                  </Stack>
                </Stack>
              </CardContent>
            </Card>
          </Grid>

          <Grid item xs={12} lg={4}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={1.5}>
                  <Stack direction="row" alignItems="center" justifyContent="space-between" gap={1}>
                    <Typography variant="h6">Library</Typography>
                    <Button
                      variant="outlined"
                      startIcon={<DownloadIcon />}
                      onClick={() => exportMutation.mutate()}
                      disabled={!activeVideo || busy}
                    >
                      Export
                    </Button>
                  </Stack>
                  <Stack spacing={1} sx={{ maxHeight: 250, overflowY: 'auto', pr: 0.5 }}>
                    {(videosQuery.data?.videos ?? []).map((video) => (
                      <Button
                        key={video.videoId}
                        variant={video.videoId === activeVideo?.videoId ? 'contained' : 'outlined'}
                        onClick={() => resetForVideo(video)}
                        sx={{ justifyContent: 'flex-start', textAlign: 'left' }}
                      >
                        {video.name} - {video.width}x{video.height}
                      </Button>
                    ))}
                    {videosQuery.data?.videos.length === 0 && (
                      <Typography color="text.secondary">No videos opened yet.</Typography>
                    )}
                  </Stack>
                </Stack>
              </CardContent>
            </Card>
          </Grid>
        </Grid>

        <Card>
          <CardContent>
            <Stack spacing={1.5}>
              <Stack direction="row" justifyContent="space-between" alignItems="center" flexWrap="wrap" gap={1}>
                <Typography variant="h6">Playback Preview</Typography>
                <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                  <Chip label={isPlaying ? 'Playing' : 'Paused'} size="small" color={isPlaying ? 'success' : 'default'} />
                  <Chip label={`${filter} conversion`} size="small" variant="outlined" />
                </Stack>
              </Stack>
              <Grid container spacing={2}>
                <Grid item xs={12} md={6}>
                  <FramePreview src={originalPreviewSrc} label="Original" />
                </Grid>
                <Grid item xs={12} md={6}>
                  <FramePreview src={previewSrc} label={filter === 'none' ? 'Converted - Original' : `Converted - ${filter}`} />
                </Grid>
              </Grid>
            </Stack>
          </CardContent>
        </Card>
      </Stack>
    </PlaceholderPage>
  );
}
