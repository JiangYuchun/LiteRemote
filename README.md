# LiteRemote

LiteRemote is an open-source, lightweight, low-latency remote assistance prototype for temporary support sessions. The project targets the same product space as TeamViewer QuickSupport, AnyDesk, and RustDesk rather than enterprise VDI stacks such as Citrix, RDS, or VMware Horizon.

## Design goals

- User-mode Win32 GUI applications for both host and client.
- No always-on Windows Service in the current phase.
- Launch-and-use workflow for temporary remote assistance.
- Low-latency video with a client-side cursor.
- Modern plane-based architecture: video, cursor, and input are independent streams.
- Open-source implementation with a small, understandable codebase.

## Current scaffold

This repository now contains the first framework for the project:

```text
LiteRemote/
├── apps/
│   ├── host/          # Win32 host skeleton and capture/cursor tick loop
│   └── client/        # Win32 client skeleton and local cursor renderer
├── docs/              # Architecture and roadmap notes
├── include/           # Shared public protocol headers
├── src/common/        # Shared protocol validation helpers
└── CMakeLists.txt     # Cross-target build definition
```

## Roadmap

1. Cursor demo with AlphaBlend and cursor shape synchronization.
2. Cursor packet serialization over TCP.
3. Desktop Duplication API capture via DXGI.
4. Input injection and input-plane synchronization.
5. H264 encoding with hardware encoders where available.
6. D3D11 renderer.
7. NAT traversal and relay support.

## Build

The GUI applications are Windows-only. Configure and build with CMake from a Visual Studio developer shell or another Windows C++20 toolchain:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug
```

On non-Windows systems CMake configures the shared protocol library only, which keeps static checks possible while the Win32 targets remain gated behind `WIN32`.
