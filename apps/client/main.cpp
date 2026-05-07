#include "literemote/protocol.h"

#include <Windows.h>
#include <windowsx.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

namespace {

constexpr wchar_t kWindowClassName[] = L"LiteRemoteClientWindow";
constexpr UINT_PTR kRenderTimerId = 1;
constexpr UINT kRenderIntervalMs = 16;

struct ClientState {
    bool tracking_mouse_leave = false;
    bool system_cursor_hidden = false;
    bool remote_cursor_visible = true;
    POINT remote_cursor_position{160, 120};
};

ClientState g_state;

void BalanceShowCursor(bool show, ClientState& state) {
    if (show && state.system_cursor_hidden) {
        while (ShowCursor(TRUE) < 0) {
        }
        state.system_cursor_hidden = false;
    } else if (!show && !state.system_cursor_hidden) {
        while (ShowCursor(FALSE) >= 0) {
        }
        state.system_cursor_hidden = true;
    }
}

void DrawRemoteCursor(HDC hdc, POINT position) {
    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    BITMAPINFO bitmap_info{};
    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = 24;
    bitmap_info.bmiHeader.biHeight = -24;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(hdc, &bitmap_info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bitmap || !bits) {
        if (bitmap) {
            DeleteObject(bitmap);
        }
        return;
    }

    auto* pixels = static_cast<std::uint32_t*>(bits);
    std::fill(pixels, pixels + (24 * 24), 0u);
    for (int y = 0; y < 24; ++y) {
        for (int x = 0; x <= y / 2 && x < 14; ++x) {
            const bool edge = x == 0 || x == y / 2;
            pixels[(y * 24) + x] = edge ? 0xFF000000u : 0xFFFFFFFFu;
        }
    }

    HDC memory_dc = CreateCompatibleDC(hdc);
    HGDIOBJ old_bitmap = SelectObject(memory_dc, bitmap);
    AlphaBlend(hdc, position.x, position.y, 24, 24, memory_dc, 0, 0, 24, 24, blend);
    SelectObject(memory_dc, old_bitmap);
    DeleteDC(memory_dc);
    DeleteObject(bitmap);
}

void PaintClient(HWND hwnd) {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT client{};
    GetClientRect(hwnd, &client);

    HBRUSH background = CreateSolidBrush(RGB(22, 28, 36));
    FillRect(hdc, &client, background);
    DeleteObject(background);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(232, 238, 247));

    RECT text_rect = client;
    text_rect.left += 24;
    text_rect.top += 24;
    const std::wstring status = L"LiteRemote Client\n"
                                L"Video plane placeholder: TCP RAW/MJPEG first, H264 later.\n"
                                L"Cursor plane: local AlphaBlend rendering avoids encoding cursor into video.\n"
                                L"Move the mouse into this window to hide the local system cursor.";
    DrawTextW(hdc, status.c_str(), static_cast<int>(status.size()), &text_rect, DT_LEFT | DT_TOP | DT_WORDBREAK);

    if (g_state.remote_cursor_visible) {
        DrawRemoteCursor(hdc, g_state.remote_cursor_position);
    }

    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE:
        SetTimer(hwnd, kRenderTimerId, kRenderIntervalMs, nullptr);
        return 0;
    case WM_MOUSEMOVE:
        g_state.remote_cursor_position.x = GET_X_LPARAM(lparam);
        g_state.remote_cursor_position.y = GET_Y_LPARAM(lparam);
        BalanceShowCursor(false, g_state);
        if (!g_state.tracking_mouse_leave) {
            TRACKMOUSEEVENT event{};
            event.cbSize = sizeof(event);
            event.dwFlags = TME_LEAVE;
            event.hwndTrack = hwnd;
            g_state.tracking_mouse_leave = TrackMouseEvent(&event) == TRUE;
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_MOUSELEAVE:
        g_state.tracking_mouse_leave = false;
        BalanceShowCursor(true, g_state);
        return 0;
    case WM_TIMER:
        if (wparam == kRenderTimerId) {
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    case WM_PAINT:
        PaintClient(hwnd);
        return 0;
    case WM_DESTROY:
        BalanceShowCursor(true, g_state);
        KillTimer(hwnd, kRenderTimerId);
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

    HWND hwnd = CreateWindowExW(0, kWindowClassName, L"LiteRemote Client", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
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
