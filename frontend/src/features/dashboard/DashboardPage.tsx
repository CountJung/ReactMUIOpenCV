import { Card, CardContent, Chip, Grid, Stack, Typography } from '@mui/material';
import { PlaceholderPage } from '../../shared/components/PlaceholderPage';

const dashboardCards = [
  { label: 'Backend', value: 'Ready', helper: 'Health API scaffolded' },
  { label: 'Remote Access', value: 'Off', helper: 'LAN mode remains opt-in' },
  { label: 'Active Jobs', value: '0', helper: 'Job queue arrives in Phase 3' },
  { label: 'Theme', value: 'Synced', helper: 'Local preference persisted' },
];

export function DashboardPage() {
  return (
    <PlaceholderPage
      title="Vision Media Testbed"
      eyebrow="Dashboard"
      status="Phase 1"
      description="A shared React UI for WebView2 desktop use and LAN browser access."
    >
      <Grid container spacing={2}>
        {dashboardCards.map((card) => (
          <Grid item xs={12} sm={6} lg={3} key={card.label}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={1}>
                  <Stack direction="row" spacing={1} alignItems="center" justifyContent="space-between">
                    <Typography color="text.secondary">{card.label}</Typography>
                    <Chip label={card.value} size="small" color="primary" variant="outlined" />
                  </Stack>
                  <Typography variant="body2" color="text.secondary">
                    {card.helper}
                  </Typography>
                </Stack>
              </CardContent>
            </Card>
          </Grid>
        ))}
      </Grid>
    </PlaceholderPage>
  );
}
