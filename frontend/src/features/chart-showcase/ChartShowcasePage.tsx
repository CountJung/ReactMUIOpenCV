import BarChartIcon from '@mui/icons-material/BarChart';
import DonutLargeIcon from '@mui/icons-material/DonutLarge';
import TimelineIcon from '@mui/icons-material/Timeline';
import { Alert, Box, Card, CardContent, Chip, Grid, Stack, Typography } from '@mui/material';
import { alpha } from '@mui/material/styles';
import { useQuery } from '@tanstack/react-query';
import { getImageResults } from '../../api/imageApi';
import { getJobs, type JobRecord } from '../../api/jobsApi';
import { getPipelines } from '../../api/pipelineApi';
import { PlaceholderPage } from '../../shared/components/PlaceholderPage';

type ChartDatum = {
  label: string;
  value: number;
};

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
  const jobsQuery = useQuery({ queryKey: ['jobs'], queryFn: getJobs, refetchInterval: 5000 });
  const imageResultsQuery = useQuery({ queryKey: ['image-results'], queryFn: getImageResults, refetchInterval: 10000 });
  const pipelinesQuery = useQuery({ queryKey: ['pipelines'], queryFn: getPipelines, refetchInterval: 10000 });

  const jobs = jobsQuery.data?.jobs ?? [];
  const results = imageResultsQuery.data?.results ?? [];
  const executions = pipelinesQuery.data?.executions ?? [];
  const statusData = toData(countBy(jobs, (job) => job.status));
  const operationData = toData(countBy(results, (result) => result.operation));
  const pipelineStatusData = toData(countBy(executions, (execution) => execution.status));

  return (
    <PlaceholderPage
      title="Chart Showcase"
      eyebrow="Analytics"
      status={`${jobs.length} jobs`}
      description="Processing statistics are derived from server-owned job records, image result metadata, and pipeline executions."
    >
      <Stack spacing={2.5}>
        {(jobsQuery.isError || imageResultsQuery.isError || pipelinesQuery.isError) && (
          <Alert severity="warning">Some chart data is not available from the backend.</Alert>
        )}

        <Grid container spacing={2}>
          <Grid item xs={12} md={4}>
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

          <Grid item xs={12} md={4}>
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

          <Grid item xs={12} md={4}>
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
