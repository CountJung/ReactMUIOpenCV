import MenuIcon from '@mui/icons-material/Menu';
import {
  AppBar,
  Box,
  BottomNavigation,
  BottomNavigationAction,
  Divider,
  Drawer,
  IconButton,
  List,
  ListItem,
  ListItemButton,
  ListItemIcon,
  ListItemText,
  Stack,
  Toolbar,
  Typography,
  useMediaQuery,
} from '@mui/material';
import { useTheme } from '@mui/material/styles';
import { useState } from 'react';
import { Outlet, useLocation, useNavigate } from 'react-router-dom';
import { tokens } from '../../theme/tokens';
import { navItems } from './navigation';

function AppNavigation({ onNavigate }: { onNavigate?: () => void }) {
  const location = useLocation();
  const navigate = useNavigate();

  return (
    <List sx={{ px: 1.5 }}>
      {navItems.map((item) => {
        const Icon = item.icon;
        const selected = item.path === '/' ? location.pathname === '/' : location.pathname.startsWith(item.path);

        return (
          <ListItem key={item.path} disablePadding sx={{ mb: 0.5 }}>
            <ListItemButton
              selected={selected}
              onClick={() => {
                navigate(item.path);
                onNavigate?.();
              }}
            >
              <ListItemIcon sx={{ minWidth: 40 }}>
                <Icon fontSize="small" />
              </ListItemIcon>
              <ListItemText primary={item.label} />
            </ListItemButton>
          </ListItem>
        );
      })}
    </List>
  );
}

function BrandBlock() {
  return (
    <Stack spacing={0.5} sx={{ px: 2, py: 2 }}>
      <Typography variant="h6" component="div">
        React MUI OpenCV
      </Typography>
      <Typography variant="body2" color="text.secondary">
        Vision and media runtime
      </Typography>
    </Stack>
  );
}

export function AppShell() {
  const theme = useTheme();
  const isDesktop = useMediaQuery(theme.breakpoints.up('md'));
  const [mobileOpen, setMobileOpen] = useState(false);
  const location = useLocation();
  const navigate = useNavigate();

  const activeItem = navItems.find((item) =>
    item.path === '/' ? location.pathname === '/' : location.pathname.startsWith(item.path),
  );

  return (
    <Box sx={{ minHeight: '100vh', bgcolor: 'background.default' }}>
      <AppBar
        position="fixed"
        color="inherit"
        elevation={0}
        sx={{
          borderBottom: 1,
          borderColor: 'divider',
          ml: { md: `${tokens.density.sidebarWidth}px` },
          width: { md: `calc(100% - ${tokens.density.sidebarWidth}px)` },
        }}
      >
        <Toolbar sx={{ minHeight: tokens.density.topbarHeight }}>
          {!isDesktop && (
            <IconButton edge="start" aria-label="Open navigation" onClick={() => setMobileOpen(true)} sx={{ mr: 1 }}>
              <MenuIcon />
            </IconButton>
          )}
          <Stack spacing={0}>
            <Typography variant="h6" component="div">
              {activeItem?.label ?? 'Dashboard'}
            </Typography>
            <Typography variant="caption" color="text.secondary">
              Local desktop and LAN-ready UI
            </Typography>
          </Stack>
        </Toolbar>
      </AppBar>

      <Box component="nav" aria-label="Application navigation">
        <Drawer
          variant="permanent"
          open
          sx={{
            display: { xs: 'none', md: 'block' },
            '& .MuiDrawer-paper': {
              width: tokens.density.sidebarWidth,
              boxSizing: 'border-box',
              borderRight: 1,
              borderColor: 'divider',
            },
          }}
        >
          <BrandBlock />
          <Divider />
          <AppNavigation />
        </Drawer>

        <Drawer
          variant="temporary"
          open={mobileOpen}
          onClose={() => setMobileOpen(false)}
          ModalProps={{ keepMounted: true }}
          sx={{
            display: { xs: 'block', md: 'none' },
            '& .MuiDrawer-paper': {
              width: tokens.density.sidebarWidth,
              boxSizing: 'border-box',
            },
          }}
        >
          <BrandBlock />
          <Divider />
          <AppNavigation onNavigate={() => setMobileOpen(false)} />
        </Drawer>
      </Box>

      <Box
        component="main"
        sx={{
          minHeight: '100vh',
          ml: { md: `${tokens.density.sidebarWidth}px` },
          pt: `${tokens.density.topbarHeight}px`,
          pb: { xs: `${tokens.density.mobileNavHeight + 16}px`, md: 0 },
        }}
      >
        <Outlet />
      </Box>

      {!isDesktop && (
        <BottomNavigation
          showLabels
          value={activeItem?.path ?? '/'}
          onChange={(_, path: string) => navigate(path)}
          sx={{
            position: 'fixed',
            bottom: 0,
            left: 0,
            right: 0,
            height: tokens.density.mobileNavHeight,
            borderTop: 1,
            borderColor: 'divider',
            zIndex: theme.zIndex.appBar,
          }}
        >
          {navItems.slice(0, 4).map((item) => {
            const Icon = item.icon;
            return <BottomNavigationAction key={item.path} label={item.label} value={item.path} icon={<Icon />} />;
          })}
        </BottomNavigation>
      )}
    </Box>
  );
}
