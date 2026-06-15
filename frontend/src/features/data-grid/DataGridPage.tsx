import ArrowDownwardIcon from '@mui/icons-material/ArrowDownward';
import ArrowUpwardIcon from '@mui/icons-material/ArrowUpward';
import SearchIcon from '@mui/icons-material/Search';
import {
  Alert,
  Box,
  Card,
  CardContent,
  Chip,
  InputAdornment,
  Stack,
  Tab,
  Table,
  TableBody,
  TableCell,
  TableContainer,
  TableHead,
  TableRow,
  Tabs,
  TextField,
  Typography,
} from '@mui/material';
import { useQuery } from '@tanstack/react-query';
import {
  flexRender,
  getCoreRowModel,
  getFilteredRowModel,
  getSortedRowModel,
  useReactTable,
  type ColumnDef,
  type SortingState,
} from '@tanstack/react-table';
import { useMemo, useState } from 'react';
import { getFileLibrary, type FileLibraryItem } from '../../api/filesApi';
import { getImageResults, type ImageResult } from '../../api/imageApi';
import { getJobs, type JobRecord } from '../../api/jobsApi';
import {
  getVideoDiagnosticsHistory,
  getVideoTrackingHistory,
  type VideoDiagnosticsRecord,
  type VideoTrackingRecord,
} from '../../api/videoApi';
import { PlaceholderPage } from '../../shared/components/PlaceholderPage';

type TabKey = 'jobs' | 'results' | 'files' | 'video-diagnostics' | 'video-tracking';

function formatDate(value?: string) {
  if (!value) {
    return 'n/a';
  }

  const date = new Date(value);
  if (Number.isNaN(date.getTime())) {
    return value;
  }

  return date.toLocaleString();
}

function formatBytes(value?: number) {
  if (value === undefined) {
    return 'n/a';
  }

  if (value < 1024) {
    return `${value} B`;
  }

  if (value < 1024 * 1024) {
    return `${(value / 1024).toFixed(1)} KB`;
  }

  return `${(value / (1024 * 1024)).toFixed(1)} MB`;
}

function statusColor(status: string): 'success' | 'warning' | 'error' | 'default' {
  if (status === 'completed') {
    return 'success';
  }
  if (status === 'queued' || status === 'running') {
    return 'warning';
  }
  if (status === 'failed' || status === 'cancelled') {
    return 'error';
  }
  return 'default';
}

function ServerTable<T>({
  columns,
  data,
  emptyLabel,
  filter,
}: {
  columns: ColumnDef<T>[];
  data: T[];
  emptyLabel: string;
  filter: string;
}) {
  const [sorting, setSorting] = useState<SortingState>([]);
  const table = useReactTable({
    columns,
    data,
    state: {
      globalFilter: filter,
      sorting,
    },
    onSortingChange: setSorting,
    getCoreRowModel: getCoreRowModel(),
    getFilteredRowModel: getFilteredRowModel(),
    getSortedRowModel: getSortedRowModel(),
  });

  return (
    <TableContainer sx={{ maxHeight: 560 }}>
      <Table size="small" stickyHeader>
        <TableHead>
          {table.getHeaderGroups().map((headerGroup) => (
            <TableRow key={headerGroup.id}>
              {headerGroup.headers.map((header) => (
                <TableCell
                  key={header.id}
                  onClick={header.column.getToggleSortingHandler()}
                  sx={{
                    cursor: header.column.getCanSort() ? 'pointer' : 'default',
                    whiteSpace: 'nowrap',
                  }}
                >
                  <Stack direction="row" spacing={0.5} alignItems="center">
                    <Box component="span">
                      {flexRender(header.column.columnDef.header, header.getContext())}
                    </Box>
                    {header.column.getIsSorted() === 'asc' && (
                      <ArrowUpwardIcon fontSize="inherit" />
                    )}
                    {header.column.getIsSorted() === 'desc' && (
                      <ArrowDownwardIcon fontSize="inherit" />
                    )}
                  </Stack>
                </TableCell>
              ))}
            </TableRow>
          ))}
        </TableHead>
        <TableBody>
          {table.getRowModel().rows.map((row) => (
            <TableRow key={row.id} hover>
              {row.getVisibleCells().map((cell) => (
                <TableCell key={cell.id}>
                  {flexRender(cell.column.columnDef.cell, cell.getContext())}
                </TableCell>
              ))}
            </TableRow>
          ))}
          {table.getRowModel().rows.length === 0 && (
            <TableRow>
              <TableCell colSpan={columns.length}>
                <Typography color="text.secondary">{emptyLabel}</Typography>
              </TableCell>
            </TableRow>
          )}
        </TableBody>
      </Table>
    </TableContainer>
  );
}

export function DataGridPage() {
  const [activeTab, setActiveTab] = useState<TabKey>('jobs');
  const [filter, setFilter] = useState('');
  const jobsQuery = useQuery({ queryKey: ['jobs'], queryFn: getJobs, refetchInterval: 5000 });
  const resultsQuery = useQuery({
    queryKey: ['image-results'],
    queryFn: getImageResults,
    refetchInterval: 10000,
  });
  const filesQuery = useQuery({
    queryKey: ['file-library'],
    queryFn: getFileLibrary,
    refetchInterval: 15000,
  });
  const diagnosticsQuery = useQuery({
    queryKey: ['video-diagnostics'],
    queryFn: getVideoDiagnosticsHistory,
    refetchInterval: 10000,
  });
  const trackingQuery = useQuery({
    queryKey: ['video-tracking'],
    queryFn: getVideoTrackingHistory,
    refetchInterval: 10000,
  });

  const jobs = jobsQuery.data?.jobs ?? [];
  const results = resultsQuery.data?.results ?? [];
  const files = filesQuery.data?.files ?? [];
  const diagnostics = diagnosticsQuery.data?.records ?? [];
  const tracking = trackingQuery.data?.records ?? [];

  const jobColumns = useMemo<ColumnDef<JobRecord>[]>(
    () => [
      { accessorKey: 'id', header: 'ID' },
      { accessorKey: 'type', header: 'Type' },
      {
        accessorKey: 'status',
        header: 'Status',
        cell: ({ getValue }) => {
          const status = String(getValue());
          return <Chip label={status} size="small" color={statusColor(status)} />;
        },
      },
      {
        accessorKey: 'progress',
        header: 'Progress',
        cell: ({ getValue }) => `${getValue<number>()}%`,
      },
      { accessorKey: 'message', header: 'Message' },
      {
        accessorKey: 'updatedAt',
        header: 'Updated',
        cell: ({ getValue }) => formatDate(getValue<string>()),
      },
    ],
    [],
  );

  const resultColumns = useMemo<ColumnDef<ImageResult>[]>(
    () => [
      { accessorKey: 'resultId', header: 'Result ID' },
      { accessorKey: 'name', header: 'Name' },
      { accessorKey: 'operation', header: 'Operation' },
      {
        id: 'size',
        header: 'Size',
        accessorFn: (row) => `${row.width} x ${row.height}`,
      },
      { accessorKey: 'channels', header: 'Channels' },
      {
        id: 'shapeCount',
        header: 'Shapes',
        accessorFn: (row) => row.metadata?.shape?.shapeCount ?? '',
      },
      {
        id: 'shapeMetric',
        header: 'Shape Metric',
        accessorFn: (row) => {
          const shape = row.metadata?.shape;
          if (!shape) {
            return '';
          }
          if (shape.lineCount !== undefined) {
            return `${shape.lineCount} lines`;
          }
          if (shape.circleCount !== undefined) {
            return `${shape.circleCount} circles`;
          }
          if (shape.largestArea !== undefined) {
            return `area ${shape.largestArea.toFixed(1)}`;
          }
          return shape.operation;
        },
      },
      { accessorKey: 'sourceType', header: 'Source' },
      {
        accessorKey: 'createdAt',
        header: 'Created',
        cell: ({ getValue }) => formatDate(getValue<string>()),
      },
    ],
    [],
  );

  const fileColumns = useMemo<ColumnDef<FileLibraryItem>[]>(
    () => [
      { accessorKey: 'id', header: 'ID' },
      { accessorKey: 'name', header: 'Name' },
      { accessorKey: 'kind', header: 'Kind' },
      { accessorKey: 'sourceType', header: 'Source' },
      {
        accessorKey: 'sizeBytes',
        header: 'Size',
        cell: ({ getValue }) => formatBytes(getValue<number | undefined>()),
      },
      {
        accessorKey: 'createdAt',
        header: 'Created',
        cell: ({ getValue }) => formatDate(getValue<string | undefined>()),
      },
    ],
    [],
  );

  const diagnosticsColumns = useMemo<ColumnDef<VideoDiagnosticsRecord>[]>(
    () => [
      { accessorKey: 'diagnosticId', header: 'ID' },
      { accessorKey: 'videoName', header: 'Video' },
      {
        id: 'size',
        header: 'Size',
        accessorFn: (row) => `${row.width} x ${row.height}`,
      },
      { accessorKey: 'frameCount', header: 'Frames' },
      {
        accessorKey: 'metadataFps',
        header: 'Metadata FPS',
        cell: ({ getValue }) => getValue<number>().toFixed(2),
      },
      {
        accessorKey: 'measuredReadFps',
        header: 'Measured FPS',
        cell: ({ getValue }) => getValue<number>().toFixed(2),
      },
      {
        accessorKey: 'operation',
        header: 'Operation',
        cell: ({ getValue }) => String(getValue() ?? 'readFps'),
      },
      {
        accessorKey: 'trackedFeatures',
        header: 'Features',
        cell: ({ getValue }) => String(getValue<number | undefined>() ?? 0),
      },
      {
        accessorKey: 'averageFlowMagnitude',
        header: 'Flow px',
        cell: ({ getValue }) => (getValue<number | undefined>() ?? 0).toFixed(2),
      },
      {
        accessorKey: 'stabilizationCropPercent',
        header: 'Crop %',
        cell: ({ getValue }) => (getValue<number | undefined>() ?? 0).toFixed(2),
      },
      {
        id: 'fpsDelta',
        header: 'Delta',
        accessorFn: (row) => row.measuredReadFps - row.metadataFps,
        cell: ({ getValue }) => getValue<number>().toFixed(2),
      },
      {
        accessorKey: 'elapsedMs',
        header: 'Elapsed',
        cell: ({ getValue }) => `${getValue<number>().toFixed(1)} ms`,
      },
      {
        id: 'sample',
        header: 'Sample',
        accessorFn: (row) => `${row.framesRead} / ${row.sampleFrames}`,
      },
      {
        accessorKey: 'createdAt',
        header: 'Created',
        cell: ({ getValue }) => formatDate(getValue<string>()),
      },
    ],
    [],
  );

  const trackingColumns = useMemo<ColumnDef<VideoTrackingRecord>[]>(
    () => [
      { accessorKey: 'trackingId', header: 'ID' },
      { accessorKey: 'videoName', header: 'Video' },
      { accessorKey: 'method', header: 'Method' },
      {
        accessorKey: 'status',
        header: 'Status',
        cell: ({ getValue }) => (
          <Chip label={String(getValue())} size="small" color={statusColor(String(getValue()))} />
        ),
      },
      {
        id: 'range',
        header: 'Range',
        accessorFn: (row) => `${row.startFrame} - ${row.endFrame}`,
      },
      { accessorKey: 'framesTracked', header: 'Frames' },
      {
        accessorKey: 'averageScore',
        header: 'Score',
        cell: ({ getValue }) => getValue<number>().toFixed(3),
      },
      {
        id: 'sourceRoi',
        header: 'ROI',
        accessorFn: (row) =>
          `${row.sourceRoi.x},${row.sourceRoi.y} ${row.sourceRoi.width}x${row.sourceRoi.height}`,
      },
      {
        id: 'frameMetadata',
        header: 'Frame Metadata',
        cell: ({ row }) => {
          const first = row.original.frames[0];
          const last = row.original.frames[row.original.frames.length - 1];
          if (!first || !last) {
            return 'n/a';
          }
          return (
            <Stack spacing={0.25}>
              <Typography variant="caption">
                {first.frameIndex}: {first.x},{first.y} {first.width}x{first.height} /{' '}
                {first.score.toFixed(2)}
              </Typography>
              <Typography variant="caption" color="text.secondary">
                {last.frameIndex}: {last.x},{last.y} {last.width}x{last.height} /{' '}
                {last.score.toFixed(2)}
              </Typography>
            </Stack>
          );
        },
      },
      {
        accessorKey: 'processingMs',
        header: 'Elapsed',
        cell: ({ getValue }) => `${getValue<number>().toFixed(1)} ms`,
      },
      {
        accessorKey: 'createdAt',
        header: 'Created',
        cell: ({ getValue }) => formatDate(getValue<string>()),
      },
    ],
    [],
  );

  const currentDataCount =
    activeTab === 'jobs'
      ? jobs.length
      : activeTab === 'results'
        ? results.length
        : activeTab === 'files'
          ? files.length
          : activeTab === 'video-diagnostics'
            ? diagnostics.length
            : tracking.length;

  return (
    <PlaceholderPage
      title="Data Grid"
      eyebrow="Tables"
      status={`${currentDataCount} rows`}
      description="Server-owned job history, image results, file library entries, and video diagnostics are browsed in sortable tables."
    >
      <Stack spacing={2}>
        {(jobsQuery.isError ||
          resultsQuery.isError ||
          filesQuery.isError ||
          diagnosticsQuery.isError ||
          trackingQuery.isError) && (
          <Alert severity="warning">Some table data is not available from the backend.</Alert>
        )}

        <Card>
          <CardContent>
            <Stack spacing={2}>
              <Stack
                direction={{ xs: 'column', md: 'row' }}
                justifyContent="space-between"
                spacing={2}
              >
                <Tabs
                  value={activeTab}
                  onChange={(_, value: TabKey) => setActiveTab(value)}
                  variant="scrollable"
                >
                  <Tab label={`Jobs (${jobs.length})`} value="jobs" />
                  <Tab label={`Image Results (${results.length})`} value="results" />
                  <Tab label={`Files (${files.length})`} value="files" />
                  <Tab
                    label={`Video Diagnostics (${diagnostics.length})`}
                    value="video-diagnostics"
                  />
                  <Tab label={`Tracking (${tracking.length})`} value="video-tracking" />
                </Tabs>
                <TextField
                  value={filter}
                  onChange={(event) => setFilter(event.target.value)}
                  placeholder="Search table"
                  size="small"
                  InputProps={{
                    startAdornment: (
                      <InputAdornment position="start">
                        <SearchIcon fontSize="small" />
                      </InputAdornment>
                    ),
                  }}
                  sx={{ minWidth: { xs: '100%', md: 280 } }}
                />
              </Stack>

              {activeTab === 'jobs' && (
                <ServerTable
                  columns={jobColumns}
                  data={jobs}
                  emptyLabel="No job records are available."
                  filter={filter}
                />
              )}
              {activeTab === 'results' && (
                <ServerTable
                  columns={resultColumns}
                  data={results}
                  emptyLabel="No image result records are available."
                  filter={filter}
                />
              )}
              {activeTab === 'files' && (
                <ServerTable
                  columns={fileColumns}
                  data={files}
                  emptyLabel="The server file library is empty."
                  filter={filter}
                />
              )}
              {activeTab === 'video-diagnostics' && (
                <ServerTable
                  columns={diagnosticsColumns}
                  data={diagnostics}
                  emptyLabel="Run Measure FPS in Video Lab to collect video diagnostics."
                  filter={filter}
                />
              )}
              {activeTab === 'video-tracking' && (
                <ServerTable
                  columns={trackingColumns}
                  data={tracking}
                  emptyLabel="Run Track ROI in Video Lab to collect object tracking metadata."
                  filter={filter}
                />
              )}
            </Stack>
          </CardContent>
        </Card>
      </Stack>
    </PlaceholderPage>
  );
}
