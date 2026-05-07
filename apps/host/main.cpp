#include "literemote/protocol.h"

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include <chrono>
#include <cstdint>
#include <string>

namespace {

constexpr wchar_t kWindowClassName[] = L"LiteRemoteHostWindow";
constexpr UINT_PTR kCaptureTimerId = 1;
constexpr UINT kCaptureIntervalMs = 16;

struct HostState {
    std::uint64_t video_sequence = 0;
    std::uint64_t cursor_sequence = 0;
    POINT last_cursor_position{};
    bool cursor_visible = true;
};

std::uint64_t NowMicros() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
}

class DesktopDuplicationCapture {
public:
    bool Initialize() {
        // Phase 3 will create the D3D11 device, select IDXGIOutput1, and call DuplicateOutput.
        return true;
    }

    void Tick(HWND hwnd, HostState& state) {
        ++state.video_sequence;
        const auto header = literemote::protocol::MakeHeader(
            literemote::protocol::Plane::Video, state.video_sequence, NowMicros(), 0);
        (void)header;

        InvalidateRect(hwnd, nullptr, FALSE);
    }
};

class CursorTracker {
public:
    void Tick(HWND hwnd, HostState& state) {
        CURSORINFO cursor_info{};
        cursor_info.cbSize = sizeof(cursor_info);
        if (!GetCursorInfo(&cursor_info)) {
            return;
        }

        state.cursor_visible = (cursor_info.flags & CURSOR_SHOWING) != 0;
        state.last_cursor_position = cursor_info.ptScreenPos;

        // Phase 1/2 will read ICONINFO, convert the cursor mask/color bitmaps to premultiplied ARGB,
        // and emit a CursorPacket independently from the video stream.
        ++state.cursor_sequence;
        literemote::protocol::CursorPacketHeader cursor_header{};
        cursor_header.cursor_id = reinterpret_cast<std::uintptr_t>(cursor_info.hCursor);
        cursor_header.pos_x = cursor_info.ptScreenPos.x;
        cursor_header.pos_y = cursor_info.ptScreenPos.y;
        cursor_header.visible = state.cursor_visible ? 1 : 0;
        cursor_header.bitmap_size = 0;
        (void)literemote::protocol::MakeHeader(
            literemote::protocol::Plane::Cursor, state.cursor_sequence, NowMicros(), sizeof(cursor_header));

        InvalidateRect(hwnd, nullptr, FALSE);
    }
};

DesktopDuplicationCapture g_capture;
CursorTracker g_cursor_tracker;
HostState g_state;

void PaintStatus(HWND hwnd) {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT client{};
    GetClientRect(hwnd, &client);

    FillRect(hdc, &client, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
    SetBkMode(hdc, TRANSPARENT);

    const std::wstring status = L"LiteRemote Host\n"
                                L"Phase 1 skeleton: Cursor plane + video plane are separated.\n"
                                L"Capture path: Desktop Duplication API (DXGI), no BitBlt/GDI capture.\n"
                                L"Cursor: shape and position packets will be sent without encoding cursor into video.";
    DrawTextW(hdc, status.c_str(), static_cast<int>(status.size()), &client, DT_LEFT | DT_TOP | DT_WORDBREAK);

    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE:
        g_capture.Initialize();
        SetTimer(hwnd, kCaptureTimerId, kCaptureIntervalMs, nullptr);
        return 0;
    case WM_TIMER:
        if (wparam == kCaptureTimerId) {
            g_capture.Tick(hwnd, g_state);
            g_cursor_tracker.Tick(hwnd, g_state);
            return 0;
        }
        break;
    case WM_PAINT:
        PaintStatus(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, kCaptureTimerId);
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command) {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = WindowProc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    window_class.lpszClassName = kWindowClassName;

    RegisterClassExW(&window_class);

    HWND hwnd = CreateWindowExW(0, kWindowClassName, L"LiteRemote Host", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                CW_USEDEFAULT, 900, 560, nullptr, nullptr, instance, nullptr);
    if (!hwnd) {
        return 1;
    }

    ShowWindow(hwnd, show_command);
    UpdateWindow(hwnd);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
