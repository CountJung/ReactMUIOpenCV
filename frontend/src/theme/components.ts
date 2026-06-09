import type { Components, Theme } from '@mui/material/styles';
import { tokens } from './tokens';

export const components: Components<Omit<Theme, 'components'>> = {
  MuiButton: {
    defaultProps: {
      variant: 'contained',
    },
    styleOverrides: {
      root: {
        borderRadius: tokens.radius.sm,
        boxShadow: 'none',
      },
    },
  },
  MuiCard: {
    styleOverrides: {
      root: {
        borderRadius: tokens.radius.md,
        boxShadow: tokens.shadow.light,
      },
    },
  },
  MuiIconButton: {
    styleOverrides: {
      root: {
        borderRadius: tokens.radius.sm,
      },
    },
  },
  MuiListItemButton: {
    styleOverrides: {
      root: {
        borderRadius: tokens.radius.sm,
      },
    },
  },
};
