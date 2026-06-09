import ContentCopyIcon from '@mui/icons-material/ContentCopy';
import LanIcon from '@mui/icons-material/Lan';
import PowerSettingsNewIcon from '@mui/icons-material/PowerSettingsNew';
import RefreshIcon from '@mui/icons-material/Refresh';
import {
  Alert,
  Box,
  Button,
  Card,
  CardContent,
  Chip,
  Divider,
  Grid,
  IconButton,
  Stack,
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableRow,
  TextField,
  Tooltip,
  Typography,
} from '@mui/material';
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import { QRCodeSVG } from 'qrcode.react';
import { useMemo, useState } from 'react';
import {
  authenticateRemoteClient,
  disableRemoteAccess,
  disconnectAllRemoteClients,
  enableRemoteAccess,
  getNetworkInfo,
  getRemoteStatus,
  rotateRemoteToken,
} from '../../api/remoteApi';
import { PlaceholderPage } from '../../shared/components/PlaceholderPage';

const remoteQueryKey = ['remote-status'];
const networkQueryKey = ['network-info'];

function useRemoteMutation<T>(mutationFn: () => Promise<T>) {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn,
    onSuccess: () => {
      void queryClient.invalidateQueries({ queryKey: remoteQueryKey });
      void queryClient.invalidateQueries({ queryKey: networkQueryKey });
    },
  });
}

export function RemoteAccessPage() {
  const queryClient = useQueryClient();
  const [authPin, setAuthPin] = useState('');
  const [authResult, setAuthResult] = useState<string | null>(null);

  const remoteStatus = useQuery({
    queryKey: remoteQueryKey,
    queryFn: getRemoteStatus,
    refetchInterval: 5000,
  });

  const networkInfo = useQuery({
    queryKey: networkQueryKey,
    queryFn: getNetworkInfo,
    refetchInterval: 10000,
  });

  const enableMutation = useRemoteMutation(enableRemoteAccess);
  const disableMutation = useRemoteMutation(disableRemoteAccess);
  const rotateMutation = useRemoteMutation(rotateRemoteToken);
  const disconnectMutation = useRemoteMutation(disconnectAllRemoteClients);

  const authMutation = useMutation({
    mutationFn: () => authenticateRemoteClient({ pin: authPin }),
    onSuccess: (result) => {
      setAuthResult(`Read-only session valid until ${result.expiresAt}`);
      setAuthPin('');
      void queryClient.invalidateQueries({ queryKey: remoteQueryKey });
    },
  });

  const status = remoteStatus.data;
  const clients = status?.clients ?? [];
  const copyText = useMemo(() => status?.connectionUrl ?? '', [status?.connectionUrl]);

  const copyConnectionUrl = () => {
    if (!copyText || !navigator.clipboard) {
      return;
    }

    void navigator.clipboard.writeText(copyText);
  };

  return (
    <PlaceholderPage
      title="Remote Access"
      eyebrow="LAN Web UI"
      status={status?.enabled ? 'Enabled' : 'Disabled'}
      description="Control LAN Web UI exposure, authentication tokens, QR connection, and connected remote clients."
    >
      <Stack spacing={2.5}>
        {status && !status.lanBound && (
          <Alert severity="warning">
            Remote access settings are prepared, but the backend is bound to {status.bindHost}. Start the backend with
            {' '}
            <code>--lan</code>
            {' '}
            to accept other devices on the LAN.
          </Alert>
        )}

        <Grid container spacing={2}>
          <Grid item xs={12} lg={7}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Stack direction="row" alignItems="center" justifyContent="space-between" flexWrap="wrap" gap={1}>
                    <Stack direction="row" alignItems="center" spacing={1}>
                      <LanIcon color="primary" />
                      <Typography variant="h6">LAN Web UI Mode</Typography>
                    </Stack>
                    <Chip
                      label={status?.enabled ? 'Enabled' : 'Disabled'}
                      color={status?.enabled ? 'success' : 'default'}
                    />
                  </Stack>

                  <Grid container spacing={1.5}>
                    <Grid item xs={12} sm={6}>
                      <TextField label="Bind Host" value={status?.bindHost ?? 'Loading'} fullWidth size="small" />
                    </Grid>
                    <Grid item xs={12} sm={6}>
                      <TextField label="Selected LAN IP" value={status?.selectedIp ?? 'Loading'} fullWidth size="small" />
                    </Grid>
                    <Grid item xs={12}>
                      <TextField
                        label="Connection URL"
                        value={status?.connectionUrl ?? ''}
                        fullWidth
                        size="small"
                        InputProps={{
                          readOnly: true,
                          endAdornment: (
                            <Tooltip title="Copy URL">
                              <span>
                                <IconButton aria-label="Copy connection URL" onClick={copyConnectionUrl} disabled={!copyText}>
                                  <ContentCopyIcon fontSize="small" />
                                </IconButton>
                              </span>
                            </Tooltip>
                          ),
                        }}
                      />
                    </Grid>
                  </Grid>

                  <Stack direction={{ xs: 'column', sm: 'row' }} spacing={1}>
                    <Button
                      startIcon={<PowerSettingsNewIcon />}
                      onClick={() => (status?.enabled ? disableMutation.mutate() : enableMutation.mutate())}
                      color={status?.enabled ? 'warning' : 'primary'}
                    >
                      {status?.enabled ? 'Disable LAN UI' : 'Enable LAN UI'}
                    </Button>
                    <Button variant="outlined" startIcon={<RefreshIcon />} onClick={() => rotateMutation.mutate()}>
                      Rotate Token
                    </Button>
                    <Button variant="outlined" color="warning" onClick={() => disconnectMutation.mutate()}>
                      Disconnect All
                    </Button>
                  </Stack>
                </Stack>
              </CardContent>
            </Card>
          </Grid>

          <Grid item xs={12} lg={5}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2} alignItems="center">
                  <Typography variant="h6" alignSelf="stretch">
                    QR Connection
                  </Typography>
                  <Box
                    sx={{
                      p: 2,
                      bgcolor: 'background.default',
                      border: 1,
                      borderColor: 'divider',
                      borderRadius: 2,
                    }}
                  >
                    {status?.connectionUrl ? <QRCodeSVG value={status.connectionUrl} size={180} /> : null}
                  </Box>
                  <Typography variant="body2" color="text.secondary" align="center">
                    Remote clients authenticate with the token URL or PIN and start as read-only.
                  </Typography>
                  <Chip label={`PIN ${status?.pin ?? '------'}`} color="secondary" />
                </Stack>
              </CardContent>
            </Card>
          </Grid>
        </Grid>

        <Grid container spacing={2}>
          <Grid item xs={12} md={5}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Typography variant="h6">Session Check</Typography>
                  <TextField
                    label="PIN"
                    value={authPin}
                    onChange={(event) => setAuthPin(event.target.value)}
                    inputProps={{ inputMode: 'numeric', maxLength: 6 }}
                    size="small"
                  />
                  <Button onClick={() => authMutation.mutate()} disabled={authPin.length === 0}>
                    Create Read-only Session
                  </Button>
                  {authResult && <Alert severity="success">{authResult}</Alert>}
                  {authMutation.isError && <Alert severity="error">PIN authentication failed.</Alert>}
                </Stack>
              </CardContent>
            </Card>
          </Grid>

          <Grid item xs={12} md={7}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                <Stack spacing={2}>
                  <Typography variant="h6">Network</Typography>
                  <Stack direction="row" spacing={1} flexWrap="wrap" useFlexGap>
                    {(networkInfo.data?.addresses ?? []).map((address) => (
                      <Chip key={address} label={address} variant="outlined" />
                    ))}
                    {networkInfo.data?.addresses.length === 0 && <Chip label="No LAN IPv4 detected" />}
                  </Stack>
                  <Divider />
                  <Typography variant="h6">Connected Clients</Typography>
                  <Table size="small">
                    <TableHead>
                      <TableRow>
                        <TableCell>Address</TableCell>
                        <TableCell>Permission</TableCell>
                        <TableCell>Expires</TableCell>
                      </TableRow>
                    </TableHead>
                    <TableBody>
                      {clients.map((client) => (
                        <TableRow key={client.id}>
                          <TableCell>{client.address}</TableCell>
                          <TableCell>{client.permission}</TableCell>
                          <TableCell>{client.expiresAt}</TableCell>
                        </TableRow>
                      ))}
                      {clients.length === 0 && (
                        <TableRow>
                          <TableCell colSpan={3}>
                            <Typography color="text.secondary">No remote clients connected.</Typography>
                          </TableCell>
                        </TableRow>
                      )}
                    </TableBody>
                  </Table>
                </Stack>
              </CardContent>
            </Card>
          </Grid>
        </Grid>
      </Stack>
    </PlaceholderPage>
  );
}
