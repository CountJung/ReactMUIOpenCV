import AccountTreeIcon from '@mui/icons-material/AccountTree';
import BarChartIcon from '@mui/icons-material/BarChart';
import CenterFocusStrongIcon from '@mui/icons-material/CenterFocusStrong';
import DashboardIcon from '@mui/icons-material/Dashboard';
import ImageIcon from '@mui/icons-material/Image';
import LanIcon from '@mui/icons-material/Lan';
import MovieIcon from '@mui/icons-material/Movie';
import SettingsIcon from '@mui/icons-material/Settings';
import SpeedIcon from '@mui/icons-material/Speed';
import TableChartIcon from '@mui/icons-material/TableChart';
import TerminalIcon from '@mui/icons-material/Terminal';
import type { SvgIconComponent } from '@mui/icons-material';

export type NavItem = {
  label: string;
  path: string;
  icon: SvgIconComponent;
};

export const navItems: NavItem[] = [
  { label: 'Dashboard', path: '/', icon: DashboardIcon },
  { label: 'Remote Access', path: '/remote-access', icon: LanIcon },
  { label: 'Image Lab', path: '/image-lab', icon: ImageIcon },
  { label: 'Contour Extractor', path: '/contour-extractor', icon: CenterFocusStrongIcon },
  { label: 'Performance Lab', path: '/performance-lab', icon: SpeedIcon },
  { label: 'Video Lab', path: '/video-lab', icon: MovieIcon },
  { label: 'Pipeline Flow', path: '/pipeline-flow', icon: AccountTreeIcon },
  { label: 'Charts', path: '/charts', icon: BarChartIcon },
  { label: 'Data Grid', path: '/data-grid', icon: TableChartIcon },
  { label: 'Logs', path: '/logs', icon: TerminalIcon },
  { label: 'Settings', path: '/settings', icon: SettingsIcon },
];
