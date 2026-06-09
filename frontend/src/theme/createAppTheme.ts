import { createTheme } from '@mui/material/styles';
import { components } from './components';
import { tokens } from './tokens';

export type ResolvedThemeMode = 'light' | 'dark';

export function createAppTheme(resolvedMode: ResolvedThemeMode) {
  return createTheme({
    palette: {
      mode: resolvedMode,
      primary: {
        main: tokens.color.primary,
      },
      secondary: {
        main: tokens.color.secondary,
      },
      success: {
        main: tokens.color.success,
      },
      warning: {
        main: tokens.color.warning,
      },
      error: {
        main: tokens.color.error,
      },
      background: {
        default: resolvedMode === 'dark' ? tokens.color.darkBackground : tokens.color.lightBackground,
        paper: resolvedMode === 'dark' ? tokens.color.darkSurface : tokens.color.lightSurface,
      },
      divider: resolvedMode === 'dark' ? tokens.color.darkBorder : tokens.color.lightBorder,
    },
    shape: {
      borderRadius: tokens.radius.md,
    },
    typography: {
      fontFamily: tokens.font.family,
      h4: {
        fontWeight: 700,
      },
      h5: {
        fontWeight: 700,
      },
      h6: {
        fontWeight: 700,
      },
      button: {
        textTransform: 'none',
        fontWeight: 700,
      },
    },
    components,
  });
}
