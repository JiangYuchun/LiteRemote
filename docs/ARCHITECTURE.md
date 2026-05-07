# LiteRemote Architecture

## Product boundary

LiteRemote is a temporary remote assistance tool. It is intentionally scoped as a user-mode Win32 application pair rather than an enterprise VDI platform. The host runs in the current interactive user session and does not install or depend on a Windows Service during the MVP phase.

## Process model

### Host

- Win32 GUI application.
- Starts when the user launches it and exits when the user closes it.
- Captures the current session only.
- Produces video-plane and cursor-plane packets.

### Client

- Win32 GUI application.
- Receives video packets and renders them in a remote window.
- Receives cursor packets and draws the remote cursor locally.
- Hides the local Windows cursor while the pointer is inside the remote window to avoid a double-cursor effect.

## Plane model

LiteRemote uses independent planes instead of VNC-style framebuffer synchronization:

| Plane | Responsibility | Initial transport |
| --- | --- | --- |
| Video | Desktop frames from Desktop Duplication API | TCP with RAW/MJPEG |
| Cursor | Cursor shape, hotspot, visibility, and position | TCP cursor packets |
| Input | Mouse and keyboard events from client to host | TCP input packets |

Separating cursor and video avoids encoding the pointer into every video frame and reduces perceived pointer latency.

## Capture strategy

The capture path is Desktop Duplication API through DXGI. BitBlt and GDI capture are intentionally excluded from the design because the target is a modern low-latency streaming pipeline.

## Cursor strategy

The host tracks cursor shape and position separately:

- Cursor shape is converted to premultiplied ARGB.
- Cursor position and visibility can be updated without resending the shape bitmap.
- Cursor packets carry `cursor_id`, dimensions, pitch, hotspot, position, visibility, format, bitmap size, and bitmap bytes.

The client draws the cursor in a separate cursor plane. The MVP renderer uses GDI `AlphaBlend`; a later phase will move rendering to D3D11.

## Double-cursor handling

Because the client renders the remote cursor locally, the native Windows cursor must not also be visible over the remote surface. The client balances `ShowCursor(FALSE)` when the mouse enters the remote window and `ShowCursor(TRUE)` when it leaves or the window is destroyed.

## Lock screen and secure desktop

The MVP does not support lock screen, Winlogon, or secure UAC desktops. A normal user-mode GUI process is isolated from the secure desktop. Future support would require a Windows Service, SYSTEM privileges, and secure-desktop capture and input handling.

## Encoding roadmap

- MVP: RAW BGRA and MJPEG for implementation simplicity.
- Later: H264.
- Hardware acceleration candidates: NVENC, Intel Quick Sync, and AMD AMF.

## Network roadmap

- MVP: TCP for simple ordered delivery.
- Later: UDP or QUIC for lower latency and congestion control.
- Later: NAT traversal and relay services for unattended network topology handling.
