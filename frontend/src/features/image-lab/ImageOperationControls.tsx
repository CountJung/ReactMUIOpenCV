import { Grid, MenuItem, Slider, Stack, TextField, Typography } from '@mui/material';
import type { ImageOperation } from '../../api/imageApi';
import type { ImageParamValue, ImageParams } from '../../store/useImageLabStore';

type ImageOperationControlsProps = {
  operation: ImageOperation;
  params: ImageParams;
  setParam: (key: string, value: ImageParamValue) => void;
};

export function ImageOperationControls({
  operation,
  params,
  setParam,
}: ImageOperationControlsProps) {
  const renderNumberField = (key: string, label: string, minimum = 0, step = 1) => (
    <TextField
      key={key}
      label={label}
      type="number"
      value={params[key] ?? minimum}
      onChange={(event) => setParam(key, Number(event.target.value))}
      inputProps={{ min: minimum, step }}
      size="small"
      fullWidth
    />
  );

  const renderSlider = (key: string, label: string, minimum: number, maximum: number, step = 1) => (
    <Stack key={key} spacing={0.5}>
      <Typography variant="body2" color="text.secondary">
        {label}
      </Typography>
      <Slider
        value={Number(params[key] ?? minimum)}
        min={minimum}
        max={maximum}
        step={step}
        valueLabelDisplay="auto"
        onChange={(_, value) => setParam(key, Array.isArray(value) ? value[0] : value)}
      />
    </Stack>
  );

  return (
    <Grid container spacing={1.5}>
      {operation === 'resize' && (
        <>
          <Grid item xs={6}>
            {renderNumberField('width', 'Width', 1)}
          </Grid>
          <Grid item xs={6}>
            {renderNumberField('height', 'Height', 1)}
          </Grid>
        </>
      )}
      {operation === 'crop' && (
        <>
          <Grid item xs={6}>
            {renderNumberField('x', 'X')}
          </Grid>
          <Grid item xs={6}>
            {renderNumberField('y', 'Y')}
          </Grid>
          <Grid item xs={6}>
            {renderNumberField('width', 'Width', 1)}
          </Grid>
          <Grid item xs={6}>
            {renderNumberField('height', 'Height', 1)}
          </Grid>
        </>
      )}
      {operation === 'rotate' && (
        <Grid item xs={12}>
          <TextField
            select
            label="Angle"
            value={params.angle ?? 90}
            onChange={(event) => setParam('angle', Number(event.target.value))}
            fullWidth
            size="small"
          >
            {[90, 180, 270].map((angle) => (
              <MenuItem key={angle} value={angle}>
                {angle}
              </MenuItem>
            ))}
          </TextField>
        </Grid>
      )}
      {operation === 'flip' && (
        <Grid item xs={12}>
          <TextField
            select
            label="Direction"
            value={params.direction ?? 'horizontal'}
            onChange={(event) => setParam('direction', event.target.value)}
            fullWidth
            size="small"
          >
            <MenuItem value="horizontal">Horizontal</MenuItem>
            <MenuItem value="vertical">Vertical</MenuItem>
            <MenuItem value="both">Both</MenuItem>
          </TextField>
        </Grid>
      )}
      {(operation === 'blur' || operation === 'gaussianBlur') && (
        <Grid item xs={12}>
          {renderSlider('kernel', 'Kernel', 1, 31, 2)}
        </Grid>
      )}
      {operation === 'sharpen' && (
        <Grid item xs={12}>
          {renderSlider('strength', 'Strength', 0.2, 4, 0.1)}
        </Grid>
      )}
      {operation === 'threshold' && (
        <Grid item xs={12}>
          {renderSlider('threshold', 'Threshold', 0, 255)}
        </Grid>
      )}
      {(operation === 'edgeDetect' || operation === 'contourDetect') && (
        <>
          <Grid item xs={12}>
            {renderSlider('low', 'Low', 0, 255)}
          </Grid>
          <Grid item xs={12}>
            {renderSlider('high', 'High', 0, 255)}
          </Grid>
        </>
      )}
      {operation === 'colorConvert' && (
        <Grid item xs={12}>
          <TextField
            select
            label="Target"
            value={params.target ?? 'hsv'}
            onChange={(event) => setParam('target', event.target.value)}
            fullWidth
            size="small"
          >
            <MenuItem value="hsv">HSV</MenuItem>
            <MenuItem value="lab">Lab</MenuItem>
            <MenuItem value="gray">Gray</MenuItem>
          </TextField>
        </Grid>
      )}
      {operation === 'featureAlign' && (
        <>
          <Grid item xs={12}>
            {renderNumberField('maxFeatures', 'Max Features', 50)}
          </Grid>
          <Grid item xs={12}>
            {renderSlider('keepRatio', 'Keep Ratio', 0.05, 1, 0.01)}
          </Grid>
        </>
      )}
      {operation === 'eccAlign' && (
        <>
          <Grid item xs={12}>
            {renderNumberField('iterations', 'Iterations', 10)}
          </Grid>
          <Grid item xs={12}>
            {renderSlider('epsilon', 'Epsilon', 0.000001, 0.01, 0.000001)}
          </Grid>
        </>
      )}
      {operation === 'calibrationBoard' && (
        <>
          <Grid item xs={4}>
            {renderNumberField('boardWidth', 'Board W', 2)}
          </Grid>
          <Grid item xs={4}>
            {renderNumberField('boardHeight', 'Board H', 2)}
          </Grid>
          <Grid item xs={4}>
            {renderNumberField('squareSize', 'Square', 0.001, 0.001)}
          </Grid>
        </>
      )}
      {(['blobCentroid', 'convexHull', 'huMoments'] as ImageOperation[]).includes(operation) && (
        <>
          <Grid item xs={12}>
            {renderSlider('threshold', 'Threshold', 0, 255)}
          </Grid>
          <Grid item xs={12}>
            <TextField
              select
              label="Foreground"
              value={params.polarity ?? 'dark'}
              onChange={(event) => setParam('polarity', event.target.value)}
              fullWidth
              size="small"
            >
              <MenuItem value="dark">Dark shapes</MenuItem>
              <MenuItem value="light">Light shapes</MenuItem>
            </TextField>
          </Grid>
          <Grid item xs={6}>
            {renderNumberField('minArea', 'Min Area', 1)}
          </Grid>
          <Grid item xs={6}>
            {renderNumberField('maxShapes', 'Max Shapes', 1)}
          </Grid>
        </>
      )}
      {operation === 'houghTransform' && (
        <>
          <Grid item xs={12}>
            <TextField
              select
              label="Mode"
              value={params.mode ?? 'lines'}
              onChange={(event) => setParam('mode', event.target.value)}
              fullWidth
              size="small"
            >
              <MenuItem value="lines">Lines</MenuItem>
              <MenuItem value="circles">Circles</MenuItem>
            </TextField>
          </Grid>
          <Grid item xs={6}>
            {renderSlider('low', 'Low', 0, 255)}
          </Grid>
          <Grid item xs={6}>
            {renderSlider('high', 'High', 0, 255)}
          </Grid>
          <Grid item xs={6}>
            {renderNumberField('threshold', 'Votes', 1)}
          </Grid>
          <Grid item xs={6}>
            {renderNumberField('maxShapes', 'Max Shapes', 1)}
          </Grid>
          {params.mode !== 'circles' ? (
            <>
              <Grid item xs={6}>
                {renderNumberField('minLineLength', 'Min Line', 1)}
              </Grid>
              <Grid item xs={6}>
                {renderNumberField('maxLineGap', 'Line Gap', 0)}
              </Grid>
            </>
          ) : (
            <>
              <Grid item xs={6}>
                {renderNumberField('minDist', 'Min Dist', 1)}
              </Grid>
              <Grid item xs={6}>
                {renderNumberField('accumulator', 'Accumulator', 1)}
              </Grid>
              <Grid item xs={6}>
                {renderNumberField('minRadius', 'Min Radius', 0)}
              </Grid>
              <Grid item xs={6}>
                {renderNumberField('maxRadius', 'Max Radius', 0)}
              </Grid>
            </>
          )}
        </>
      )}
      {operation === 'inpaint' && (
        <>
          <Grid item xs={12}>
            <TextField
              select
              label="Mask"
              value={params.maskMode ?? 'edges'}
              onChange={(event) => setParam('maskMode', event.target.value)}
              fullWidth
              size="small"
            >
              <MenuItem value="edges">Edges</MenuItem>
              <MenuItem value="bright">Bright pixels</MenuItem>
              <MenuItem value="dark">Dark pixels</MenuItem>
              <MenuItem value="center">Center rectangle</MenuItem>
            </TextField>
          </Grid>
          <Grid item xs={12}>
            {renderSlider('radius', 'Radius', 1, 24)}
          </Grid>
          {params.maskMode !== 'edges' && params.maskMode !== 'center' && (
            <Grid item xs={12}>
              {renderSlider('threshold', 'Threshold', 0, 255)}
            </Grid>
          )}
          {params.maskMode === 'center' && (
            <>
              <Grid item xs={6}>
                {renderNumberField('width', 'Mask W', 4)}
              </Grid>
              <Grid item xs={6}>
                {renderNumberField('height', 'Mask H', 4)}
              </Grid>
            </>
          )}
        </>
      )}
      {operation === 'seamlessClone' && (
        <>
          <Grid item xs={12}>
            <TextField
              select
              label="Mode"
              value={params.mode ?? 'mixed'}
              onChange={(event) => setParam('mode', event.target.value)}
              fullWidth
              size="small"
            >
              <MenuItem value="mixed">Mixed clone</MenuItem>
              <MenuItem value="normal">Normal clone</MenuItem>
            </TextField>
          </Grid>
          {(['x', 'y', 'width', 'height'] as const).map((key) => (
            <Grid key={key} item xs={6}>
              {renderNumberField(
                key,
                key === 'width' ? 'Source W' : key === 'height' ? 'Source H' : key.toUpperCase(),
                key === 'width' || key === 'height' ? 4 : 0,
              )}
            </Grid>
          ))}
          <Grid item xs={6}>
            {renderNumberField('targetX', 'Target X', 0)}
          </Grid>
          <Grid item xs={6}>
            {renderNumberField('targetY', 'Target Y', 0)}
          </Grid>
        </>
      )}
      {operation === 'alphaBlend' && (
        <Grid item xs={12}>
          {renderSlider('alpha', 'Alpha', 0, 1, 0.01)}
        </Grid>
      )}
      {operation === 'exposureFusion' && (
        <>
          <Grid item xs={6}>
            {renderSlider('darkGamma', 'Dark Gamma', 0.2, 4, 0.05)}
          </Grid>
          <Grid item xs={6}>
            {renderSlider('brightGamma', 'Bright Gamma', 0.2, 4, 0.05)}
          </Grid>
          <Grid item xs={4}>
            {renderSlider('contrast', 'Contrast', 0, 3, 0.05)}
          </Grid>
          <Grid item xs={4}>
            {renderSlider('saturation', 'Saturation', 0, 3, 0.05)}
          </Grid>
          <Grid item xs={4}>
            {renderSlider('exposure', 'Exposure', 0, 3, 0.05)}
          </Grid>
        </>
      )}
      {operation === 'hdrTonemap' && (
        <>
          <Grid item xs={6}>
            {renderSlider('exposureScale', 'Exposure Scale', 0.5, 8, 0.1)}
          </Grid>
          <Grid item xs={6}>
            {renderSlider('gamma', 'Gamma', 0.2, 3, 0.05)}
          </Grid>
          <Grid item xs={12}>
            {renderSlider('intensity', 'Intensity', -8, 8, 0.1)}
          </Grid>
          <Grid item xs={6}>
            {renderSlider('lightAdapt', 'Light Adapt', 0, 1, 0.01)}
          </Grid>
          <Grid item xs={6}>
            {renderSlider('colorAdapt', 'Color Adapt', 0, 1, 0.01)}
          </Grid>
        </>
      )}
      {(operation === 'stylization' || operation === 'pencilSketch') && (
        <>
          <Grid item xs={6}>
            {renderSlider('sigmaS', 'Sigma S', 1, 200)}
          </Grid>
          <Grid item xs={6}>
            {renderSlider('sigmaR', 'Sigma R', 0.01, 1, 0.01)}
          </Grid>
          {operation === 'pencilSketch' && (
            <>
              <Grid item xs={6}>
                {renderSlider('shade', 'Shade', 0, 0.2, 0.005)}
              </Grid>
              <Grid item xs={6}>
                <TextField
                  select
                  label="Mode"
                  value={params.mode ?? 'color'}
                  onChange={(event) => setParam('mode', event.target.value)}
                  fullWidth
                  size="small"
                >
                  <MenuItem value="color">Color</MenuItem>
                  <MenuItem value="gray">Gray</MenuItem>
                </TextField>
              </Grid>
            </>
          )}
        </>
      )}
    </Grid>
  );
}
