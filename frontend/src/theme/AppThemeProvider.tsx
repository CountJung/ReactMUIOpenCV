import CssBaseline from '@mui/material/CssBaseline';
import { ThemeProvider } from '@mui/material/styles';
import { useEffect, useMemo, useState, type PropsWithChildren } from 'react';
import { getSystemPrefersDark, isBrowser } from '../shared/utils/browser';
import { createAppTheme } from './createAppTheme';
import { ThemeModeContext, type ThemeModeContextValue } from './ThemeModeContext';
import { readStoredThemeMode, writeStoredThemeMode, type ThemeMode } from './themeMode';

export function AppThemeProvider({ children }: PropsWithChildren) {
  const [mode, setModeState] = useState<ThemeMode>(() => readStoredThemeMode());
  const [prefersDark, setPrefersDark] = useState(() => getSystemPrefersDark());

  useEffect(() => {
    if (!isBrowser() || !window.matchMedia) {
      return undefined;
    }

    const query = window.matchMedia('(prefers-color-scheme: dark)');
    const updatePreference = () => setPrefersDark(query.matches);

    updatePreference();
    query.addEventListener('change', updatePreference);

    return () => query.removeEventListener('change', updatePreference);
  }, []);

  const resolvedMode = mode === 'system' ? (prefersDark ? 'dark' : 'light') : mode;
  const theme = useMemo(() => createAppTheme(resolvedMode), [resolvedMode]);

  const value = useMemo<ThemeModeContextValue>(
    () => ({
      mode,
      resolvedMode,
      setMode: (nextMode) => {
        setModeState(nextMode);
        writeStoredThemeMode(nextMode);
      },
    }),
    [mode, resolvedMode],
  );

  return (
    <ThemeModeContext.Provider value={value}>
      <ThemeProvider theme={theme}>
        <CssBaseline />
        {children}
      </ThemeProvider>
    </ThemeModeContext.Provider>
  );
}
