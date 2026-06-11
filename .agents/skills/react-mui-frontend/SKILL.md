---
name: react-mui-frontend
description: Frontend implementation guidance for this project's React + Vite + TypeScript + MUI UI. Use when Codex edits TS/TSX frontend files, creates routes or app shell UI, implements the MUI theme system, API clients, runtime adapters, WebSocket clients, TanStack Query sync, React Flow screens, responsive desktop/tablet/mobile layouts, browser verification, or feature pages such as Dashboard, Remote Access, Image Lab, Video Lab, Logs, Settings, Charts, or Data Grid.
---

# React MUI Frontend

Use this skill to build the shared UI that runs both inside WebView2 and in LAN browsers. Keep components portable by routing all platform differences through runtime adapters and all backend access through typed API clients.

## Workflow

1. Read `AGENTS.md`, `TODO.md`, and relevant `MasterPlan.md` sections before larger edits.
2. Place code under the planned folders:
   - `src/app` for app root, routing, and providers.
   - `src/theme` for MUI theme tokens and providers.
   - `src/api` for HTTP and WebSocket clients.
   - `src/runtime` for Desktop vs LAN behavior.
   - `src/store` for client-owned UI state.
   - `src/features` for route-level features.
   - `src/shared` for reusable components, hooks, layouts, utils, and types.
3. Keep feature work vertical: route, UI state, API client, loading/error states, and responsive behavior.

## Theme Rules

- Use theme tokens for color, typography, shape, shadows, density, and component overrides.
- Support `light`, `dark`, and `system`.
- Do not read `localStorage`, `window`, `document`, or `matchMedia` directly in render bodies.
- Apply `ThemeProvider` and `CssBaseline` once near the app root.
- Sync theme settings to local storage first and backend settings when the settings API exists.

## Runtime And API Rules

- Never call WebView2-specific APIs from feature components.
- Use `runtime/fileAdapter.ts` for local open/upload differences.
- Use `api/client.ts` for REST conventions and `api/wsClient.ts` for WebSocket event handling.
- Treat jobs, progress, results, logs, clients, and remote access as server-owned state.
- Use TanStack Query cache updates or invalidation after WebSocket events.

## Responsive Rules

- Desktop: sidebar, top toolbar, split panes, large preview, detailed parameter panels.
- Tablet: collapsible sidebar, bottom-sheet parameter panels, touch-friendly controls.
- Mobile: bottom navigation, status, previews, logs, and simple start/stop actions.
- Restrict complex node editing, large video upload, timeline editing, arbitrary path save, and system settings on mobile.

## Verification

- Run available `package.json` lint, typecheck, test, or build scripts after meaningful edits.
- Visually verify desktop and mobile breakpoints for UI work.
- Prefer the Browser plugin's in-app browser when available. If `agent.browsers.get("iab")` reports `Browser is not available: iab`, use `mcp__playwright` browser tools as the internal-browser fallback for localhost UI checks.
- Confirm text fits controls and no panels overlap.
