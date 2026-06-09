---
name: webview2-lan-runtime
description: Runtime guidance for WebView2 desktop hosting and LAN browser access in this project. Use when Codex implements or changes WebView2 startup, local server URL policy, 127.0.0.1 vs 0.0.0.0 binding, LAN IP discovery, QR connection URLs, remote access enable/disable flows, connected-client tracking, PIN/token sessions, firewall guidance, or frontend runtime adapters for Desktop vs LAN behavior.
---

# WebView2 LAN Runtime

Use this skill when work touches how the same React UI runs inside the Windows WebView2 host and from other devices on the local network.

## Runtime Model

- Desktop mode loads `http://127.0.0.1:18730` in WebView2.
- LAN Web UI Mode is off by default.
- Enabling LAN mode switches backend binding to `0.0.0.0`, exposes a LAN URL, and requires authentication.
- LAN clients start in read-only mode unless Control Mode is explicitly granted.
- Restricted actions stay unavailable to remote clients.

## Backend Work

- Implement binding changes in backend server/runtime code, not in React components.
- Expose network information through API, including the selected LAN IP and active port.
- Track client connections, permission mode, last seen time, and disconnect events.
- Implement `status`, `enable`, `disable`, `rotate-token`, `clients`, and `disconnect-all` APIs.
- Emit `remote.client.connected` and `remote.client.disconnected` WebSocket events.

## Frontend Work

- Put Desktop vs LAN differences in `frontend/src/runtime`.
- Keep Remote Access UI focused on mode, URL, QR code, token/PIN status, client list, permission mode, and safety warnings.
- Never let feature components assume they are running inside WebView2.
- Hide or disable restricted actions for remote clients.

## Security Checks

- Require explicit user action before external binding.
- Require PIN or temporary token authentication.
- Apply session timeout.
- Show connected clients and provide Disconnect All.
- Warn about public Wi-Fi and Windows firewall behavior.
- Verify every API request against the client's permission mode.

## Verification

- Verify desktop mode binds only to `127.0.0.1`.
- Verify LAN mode requires auth before UI control.
- Verify read-only clients cannot start jobs, change settings, delete files, or access arbitrary paths.
- Verify QR and displayed URLs use the actual selected LAN IP.
