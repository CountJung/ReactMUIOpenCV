import { Box, Card, CardContent, Chip, Stack, Typography } from '@mui/material';
import type { ReactNode } from 'react';

type PlaceholderPageProps = {
  title: string;
  eyebrow: string;
  description: string;
  status?: string;
  children?: ReactNode;
};

export function PlaceholderPage({ title, eyebrow, description, status = 'Planned', children }: PlaceholderPageProps) {
  return (
    <Box sx={{ p: { xs: 2, md: 3 }, maxWidth: 1180 }}>
      <Stack spacing={2.5}>
        <Stack spacing={1}>
          <Stack direction="row" spacing={1} alignItems="center" flexWrap="wrap">
            <Typography variant="overline" color="text.secondary">
              {eyebrow}
            </Typography>
            <Chip label={status} size="small" color="primary" variant="outlined" />
          </Stack>
          <Typography variant="h4" component="h1">
            {title}
          </Typography>
          <Typography color="text.secondary" sx={{ maxWidth: 760 }}>
            {description}
          </Typography>
        </Stack>

        {children ?? (
          <Card>
            <CardContent>
              <Typography color="text.secondary">
                This route is wired into the app shell and ready for feature implementation.
              </Typography>
            </CardContent>
          </Card>
        )}
      </Stack>
    </Box>
  );
}
