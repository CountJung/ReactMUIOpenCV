import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import type { PropsWithChildren } from 'react';
import { BackendEventBridge } from './BackendEventBridge';
import { AppThemeProvider } from '../theme/AppThemeProvider';

const queryClient = new QueryClient({
  defaultOptions: {
    queries: {
      refetchOnWindowFocus: false,
      retry: 1,
    },
  },
});

export function AppProviders({ children }: PropsWithChildren) {
  return (
    <QueryClientProvider client={queryClient}>
      <BackendEventBridge />
      <AppThemeProvider>{children}</AppThemeProvider>
    </QueryClientProvider>
  );
}
