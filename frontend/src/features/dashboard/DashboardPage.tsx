import AccessTimeIcon from '@mui/icons-material/AccessTime';
import ImageIcon from '@mui/icons-material/Image';
import LanIcon from '@mui/icons-material/Lan';
import MemoryIcon from '@mui/icons-material/Memory';
import PlayCircleIcon from '@mui/icons-material/PlayCircle';
import SpeedIcon from '@mui/icons-material/Speed';
import WarningAmberIcon from '@mui/icons-material/WarningAmber';
import {
  Alert,
  Box,
  Card,
  CardContent,
  Chip,
  Divider,
  Grid,
  LinearProgress,
  Stack,
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableRow,
  Typography,
} from '@mui/material';
import type { SvgIconComponent } from '@mui/icons-material';
import { useQuery } from '@tanstack/react-query';
import { getHealth, getServerInfo } from '../../api/client';
import { absoluteImageUrl, getImageResults } from '../../api/imageApi';
import { getJobs, type JobRecord } from '../../api/jobsApi';
import { getRecentLogs, type LogEntry } from '../../api/logsApi';
import { getPerformanceBenchmarks } from '../../api/performanceApi';
import { getPipelines } from '../../api/pipelineApi';
import { getRemoteStatus } from '../../api/remoteApi';
import { PlaceholderPage } from '../../shared/components/PlaceholderPage';

function statusColor(status: string): 'success' | 'warning' | 'error' | 'default' {
  if (status === 'completed' || status === 'ok') {
    return 'success';
  }
  if (status === 'running' || status === 'queued') {
    return 'warning';
  }
  if (status === 'failed' || status === 'error') {
    return 'error';
  }
  return 'default';
}

function logColor(level: string): 'warning' | 'error' | 'default' {
  if (level === 'error') {
    return 'error';
  }
  if (level === 'warning') {
    return 'warning';
  }
  return 'default';
}

function formatTime(value?: string) {
  if (!value) {
    return 'n/a';
  }

  const date = new Date(value);
  if (Number.isNaN(date.getTime())) {
    return value;
  }

  return date.toLocaleString();
}

function MetricCard({
  icon: Icon,
  label,
  value,
  helper,
  color = 'primary',
}: {
  icon: SvgIconComponent;
  label: string;
  value: string;
  helper: string;
  color?: 'primary' | 'success' | 'warning' | 'error';
}) {
  return (
    <Card sx={{ height: '100%' }}>
      <CardContent>
        <Stack spacing={1.25}>
          <Stack direction="row" alignItems="center" justifyContent="space-between" spacing={1}>
            <Typography variant="body2" color="text.secondary">
              {label}
            </Typography>
            <Icon color={color} fontSize="small" />
          </Stack>
          <Typography variant="h5">{value}</Typography>
          <Typography variant="caption" color="text.secondary">
            {helper}
          </Typography>
        </Stack>
      </CardContent>
    </Card>
  );
}

function JobList({ jobs }: { jobs: JobRecord[] }) {
  const rows = jobs.slice(0, 6);

  return (
    <Table size="small">
      <TableHead>
        <TableRow>
          <TableCell>Job</TableCell>
          <TableCell>Status</TableCell>
          <TableCell>Progress</TableCell>
          <TableCell>Updated</TableCell>
        </TableRow>
      </TableHead>
      <TableBody>
        {rows.map((job) => (
          <TableRow key={job.id}>
            <TableCell>
              <Stack spacing={0.25}>
                <Typography variant="body2">{job.type}</Typography>
                <Typography variant="caption" color="text.secondary">
                  {job.id}
                </Typography>
              </Stack>
            </TableCell>
            <TableCell>
              <Chip label={job.status} size="small" color={statusColor(job.status)} />
            </TableCell>
            <TableCell sx={{ minWidth: 150 }}>
              <Stack spacing={0.5}>
                <LinearProgress
                  variant="determinate"
                  value={Math.max(0, Math.min(100, job.progress))}
                />
                <Typography variant="caption" color="text.secondary">
                  {job.progress}%
                </Typography>
              </Stack>
            </TableCell>
            <TableCell>{formatTime(job.updatedAt)}</TableCell>
          </TableRow>
        ))}
        {rows.length === 0 && (
          <TableRow>
            <TableCell colSpan={4}>
              <Typography color="text.secondary">No jobs have been recorded.</Typography>
            </TableCell>
          </TableRow>
        )}
      </TableBody>
    </Table>
  );
}

function LogList({ logs }: { logs: LogEntry[] }) {
  const rows = logs.slice(0, 5);

  return (
    <Stack spacing={1}>
      {rows.map((log) => (
        <Stack key={log.id} direction="row" spacing={1} alignItems="flex-start">
          <Chip label={log.level} size="small" color={logColor(log.level)} sx={{ minWidth: 76 }} />
          <Stack spacing={0.25}>
            <Typography variant="body2">{log.message}</Typography>
            <Typography variant="caption" color="text.secondary">
              {formatTime(log.timestamp)}
            </Typography>
          </Stack>
        </Stack>
      ))}
      {rows.length === 0 && (
        <Typography color="text.secondary">No recent logs are available.</Typography>
      )}
    </Stack>
  );
}

export function DashboardPage() {
  const healthQuery = useQuery({
    queryKey: ['health'],
    queryFn: getHealth,
    refetchInterval: 10000,
  });
  const serverInfoQuery = useQuery({
    queryKey: ['server-info'],
    queryFn: getServerInfo,
    refetchInterval: 15000,
  });
  const remoteStatusQuery = useQuery({
    queryKey: ['remote-status'],
    queryFn: getRemoteStatus,
    refetchInterval: 5000,
  });
  const jobsQuery = useQuery({ queryKey: ['jobs'], queryFn: getJobs, refetchInterval: 5000 });
  const logsQuery = useQuery({
    queryKey: ['logs'],
    queryFn: getRecentLogs,
    refetchInterval: 10000,
  });
  const imageResultsQuery = useQuery({
    queryKey: ['image-results'],
    queryFn: getImageResults,
    refetchInterval: 10000,
  });
  const pipelinesQuery = useQuery({
    queryKey: ['pipelines'],
    queryFn: getPipelines,
    refetchInterval: 10000,
  });
  const benchmarksQuery = useQuery({
    queryKey: ['performance-benchmarks'],
    queryFn: getPerformanceBenchmarks,
    refetchInterval: 10000,
  });

  const jobs = jobsQuery.data?.jobs ?? [];
  const logs = logsQuery.data?.logs ?? [];
  const results = imageResultsQuery.data?.results ?? [];
  const activeJobs = jobs.filter((job) => job.status === 'queued' || job.status === 'running');
  const errorLogs = logs.filter((log) => log.level === 'error');
  const remoteStatus = remoteStatusQuery.data;
  const latestResult = results[0];
  const benchmarks = benchmarksQuery.data?.records ?? [];
  const latestBenchmark = benchmarks[0];

  return (
    <PlaceholderPage
      title="Vision Media Testbed"
      eyebrow="Dashboard"
      status={healthQuery.data?.status ?? 'Checking'}
      description="Server-owned runtime state for the local C++ OpenCV backend, remote clients, jobs, logs, and recent media results."
    >
      <Stack spacing={2.5}>
        {(healthQuery.isError ||
          jobsQuery.isError ||
          logsQuery.isError ||
          benchmarksQuery.isError) && (
          <Alert severity="warning">Some backend dashboard data is not available yet.</Alert>
        )}

        <Grid container spacing={2}>
          <Grid item xs={12} sm={6} lg={3}>
            <MetricCard
              icon={MemoryIcon}
              label="Backend"
              value={healthQuery.data?.service ?? 'Offline'}
              helper={`OpenCV ${healthQuery.data?.opencvVersion ?? 'unknown'}`}
              color={healthQuery.data?.status === 'ok' ? 'success' : 'warning'}
            />
          </Grid>
          <Grid item xs={12} sm={6} lg={3}>
            <MetricCard
              icon={PlayCircleIcon}
              label="Active Jobs"
              value={String(activeJobs.length)}
              helper={`${jobs.length} total job records`}
              color={activeJobs.length > 0 ? 'warning' : 'success'}
            />
          </Grid>
          <Grid item xs={12} sm={6} lg={3}>
            <MetricCard
              icon={LanIcon}
              label="Remote Clients"
              value={String(remoteStatus?.clients.length ?? 0)}
              helper={remoteStatus?.enabled ? remoteStatus.connectionUrl : 'LAN Web UI disabled'}
              color={remoteStatus?.enabled ? 'success' : 'primary'}
            />
          </Grid>
          <Grid item xs={12} sm={6} lg={3}>
            <MetricCard
              icon={WarningAmberIcon}
              label="Recent Errors"
              value={String(errorLogs.length)}
              helper={`${logs.length} recent log entries`}
              color={errorLogs.length > 0 ? 'error' : 'success'}
            />
          </Grid>
          <Grid item xs={12} sm={6} lg={3}>
            <MetricCard
              icon={SpeedIcon}
              label="Benchmarks"
              value={String(benchmarks.length)}
              helper={
                latestBenchmark
                  ? `${latestBenchmark.fastestMethod}, ${latestBenchmark.forEachSpeedupVsPointer.toFixed(2)}x forEach`
                  : 'No OpenCV pixel samples yet'
              }
              color={benchmarks.length > 0 ? 'success' : 'primary'}
            />
          </Grid>
        </Grid>

        <Grid container spacing={2}>
          <Grid item xs={12} lg={8}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack
                    direction="row"
                    justifyContent="space-between"
                    alignItems="center"
                    flexWrap="wrap"
                    gap={1}
                  >
                    <Typography variant="h6">Current Jobs</Typography>
                    <Chip label={`${jobs.length} records`} size="small" variant="outlined" />
                  </Stack>
                  <JobList jobs={jobs} />
                </Stack>
              </CardContent>
            </Card>
          </Grid>

          <Grid item xs={12} lg={4}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Typography variant="h6">Runtime Endpoints</Typography>
                  <Stack spacing={1}>
                    <Typography variant="body2">
                      HTTP:{' '}
                      <Box component="span" color="text.secondary">
                        {serverInfoQuery.data?.httpUrl ?? 'n/a'}
                      </Box>
                    </Typography>
                    <Typography variant="body2">
                      WebSocket:{' '}
                      <Box component="span" color="text.secondary">
                        {serverInfoQuery.data?.wsUrl ?? 'n/a'}
                      </Box>
                    </Typography>
                    <Typography variant="body2">
                      Static root:{' '}
                      <Box component="span" color="text.secondary">
                        {serverInfoQuery.data?.staticRoot ?? 'n/a'}
                      </Box>
                    </Typography>
                  </Stack>
                  <Divider />
                  <Typography variant="h6">Recent Logs</Typography>
                  <LogList logs={logs} />
                </Stack>
              </CardContent>
            </Card>
          </Grid>
        </Grid>

        <Grid container spacing={2}>
          <Grid item xs={12} md={5}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack direction="row" alignItems="center" spacing={1}>
                    <ImageIcon color="primary" />
                    <Typography variant="h6">Latest Result</Typography>
                  </Stack>
                  {latestResult ? (
                    <>
                      <Box
                        component="img"
                        src={absoluteImageUrl(latestResult.previewUrl)}
                        alt={latestResult.name}
                        sx={{
                          bgcolor: 'background.default',
                          border: 1,
                          borderColor: 'divider',
                          borderRadius: 2,
                          maxHeight: 220,
                          objectFit: 'contain',
                          width: '100%',
                        }}
                      />
                      <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                        <Chip label={latestResult.operation} size="small" color="primary" />
                        <Chip
                          label={`${latestResult.width} x ${latestResult.height}`}
                          size="small"
                          variant="outlined"
                        />
                      </Stack>
                    </>
                  ) : (
                    <Typography color="text.secondary">
                      No image results have been produced yet.
                    </Typography>
                  )}
                </Stack>
              </CardContent>
            </Card>
          </Grid>

          <Grid item xs={12} md={7}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack direction="row" alignItems="center" spacing={1}>
                    <AccessTimeIcon color="primary" />
                    <Typography variant="h6">Pipeline Summary</Typography>
                  </Stack>
                  <Grid container spacing={1.5}>
                    <Grid item xs={12} sm={4}>
                      <MetricCard
                        icon={PlayCircleIcon}
                        label="Saved Pipelines"
                        value={String(pipelinesQuery.data?.pipelines.length ?? 0)}
                        helper="Server-owned documents"
                      />
                    </Grid>
                    <Grid item xs={12} sm={4}>
                      <MetricCard
                        icon={AccessTimeIcon}
                        label="Executions"
                        value={String(pipelinesQuery.data?.executions.length ?? 0)}
                        helper="Recent pipeline runs"
                      />
                    </Grid>
                    <Grid item xs={12} sm={4}>
                      <MetricCard
                        icon={ImageIcon}
                        label="Results"
                        value={String(results.length)}
                        helper="Image result library"
                      />
                    </Grid>
                  </Grid>
                </Stack>
              </CardContent>
            </Card>
          </Grid>
        </Grid>
      </Stack>
    </PlaceholderPage>
  );
}
