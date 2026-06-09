import { createBrowserRouter } from 'react-router-dom';
import { ChartShowcasePage } from '../features/chart-showcase/ChartShowcasePage';
import { DataGridPage } from '../features/data-grid/DataGridPage';
import { DashboardPage } from '../features/dashboard/DashboardPage';
import { ImageLabPage } from '../features/image-lab/ImageLabPage';
import { LogsPage } from '../features/logs/LogsPage';
import { PipelineFlowPage } from '../features/pipeline-flow/PipelineFlowPage';
import { RemoteAccessPage } from '../features/remote-access/RemoteAccessPage';
import { SettingsPage } from '../features/settings/SettingsPage';
import { VideoLabPage } from '../features/video-lab/VideoLabPage';
import { AppShell } from '../shared/layouts/AppShell';

export const router = createBrowserRouter([
  {
    path: '/',
    element: <AppShell />,
    children: [
      { index: true, element: <DashboardPage /> },
      { path: 'remote-access', element: <RemoteAccessPage /> },
      { path: 'image-lab', element: <ImageLabPage /> },
      { path: 'video-lab', element: <VideoLabPage /> },
      { path: 'pipeline-flow', element: <PipelineFlowPage /> },
      { path: 'charts', element: <ChartShowcasePage /> },
      { path: 'data-grid', element: <DataGridPage /> },
      { path: 'logs', element: <LogsPage /> },
      { path: 'settings', element: <SettingsPage /> },
    ],
  },
]);
