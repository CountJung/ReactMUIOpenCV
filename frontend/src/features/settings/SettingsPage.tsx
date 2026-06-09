import {
  Card,
  CardContent,
  FormControl,
  FormControlLabel,
  FormLabel,
  Radio,
  RadioGroup,
  Stack,
  Typography,
} from '@mui/material';
import { updateSettings } from '../../api/settingsApi';
import { PlaceholderPage } from '../../shared/components/PlaceholderPage';
import { useThemeMode, type ThemeMode } from '../../theme';

const themeOptions: Array<{ value: ThemeMode; label: string; helper: string }> = [
  { value: 'light', label: 'Light', helper: 'Use the light theme at all times.' },
  { value: 'dark', label: 'Dark', helper: 'Use the dark theme at all times.' },
  { value: 'system', label: 'System', helper: 'Follow the operating system preference.' },
];

export function SettingsPage() {
  const { mode, resolvedMode, setMode } = useThemeMode();

  const handleThemeChange = (nextMode: ThemeMode) => {
    setMode(nextMode);
    void updateSettings({ themeMode: nextMode });
  };

  return (
    <PlaceholderPage
      title="Settings"
      eyebrow="Preferences"
      status={`Resolved ${resolvedMode}`}
      description="Application preferences that should remain consistent between desktop and LAN clients."
    >
      <Card>
        <CardContent>
          <Stack spacing={2}>
            <FormControl>
              <FormLabel id="theme-mode-label">Theme Mode</FormLabel>
              <RadioGroup
                aria-labelledby="theme-mode-label"
                value={mode}
                onChange={(event) => handleThemeChange(event.target.value as ThemeMode)}
              >
                {themeOptions.map((option) => (
                  <FormControlLabel
                    key={option.value}
                    value={option.value}
                    control={<Radio />}
                    label={
                      <Stack spacing={0}>
                        <Typography>{option.label}</Typography>
                        <Typography variant="caption" color="text.secondary">
                          {option.helper}
                        </Typography>
                      </Stack>
                    }
                  />
                ))}
              </RadioGroup>
            </FormControl>
          </Stack>
        </CardContent>
      </Card>
    </PlaceholderPage>
  );
}
