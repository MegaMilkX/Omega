#include "platform.hpp"

#include <assert.h>
#include <stack>
#include <Windows.h>
#include <gl/GL.h>

#include "platform/gl/glextutil.h"
#include "log/log.hpp"

#include "gui/gui_draw.hpp"

static bool s_is_running = true;

static HWND s_hWnd;
static HDC s_hdc = 0;
static HGLRC s_gl_context;

static int s_window_width = 1920, s_window_height = 1080;
static int s_windowed_width = 1920, s_windowed_height = 1080;
static bool s_is_fullscreen = false;
static WINDOWPLACEMENT s_window_placement = { 0 };
static gfxm::rect s_viewport_rect(gfxm::vec2(0, 0), gfxm::vec2(1920, 1080));
static platform_window_resize_cb_t s_window_resize_cb_f = 0;

static bool s_is_mouse_locked = false;
static bool s_is_mouse_hidden = false;

static GLint maxAttribs = 0;
static GLint maxUniformBufferBindings = 0;
static GLint maxTextureBufferSz = 0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProcToolGui(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void APIENTRY glDbgCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void*) {
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        LOG_ERR(std::string(message, length));
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        LOG_WARN(std::string(message, length));
        break;
    case GL_DEBUG_SEVERITY_LOW:
        LOG_WARN(std::string(message, length));
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        //LOG(std::string(message, length));
        break;
    }
}

int platformInit(bool show_window, bool tooling_gui_enabled) {
    WNDCLASSEXA wc_tmp = { 0 };
    wc_tmp.cbSize = sizeof(wc_tmp);
    wc_tmp.lpfnWndProc = DefWindowProc;
    wc_tmp.hInstance = GetModuleHandle(0);
    wc_tmp.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    wc_tmp.lpszClassName = "MainWindowTmp";
    wc_tmp.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    if (!RegisterClassExA(&wc_tmp)) {
        return 1;
    }
    HWND hWnd_tmp = CreateWindowA(wc_tmp.lpszClassName, "Omega", WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, 0, 0, wc_tmp.hInstance, 0);

    PIXELFORMATDESCRIPTOR pfd_tmp =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
        PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
        32,                   // Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,                   // Number of bits for the depthbuffer
        8,                    // Number of bits for the stencilbuffer
        0,                    // Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };
    HDC hdc_tmp = GetDC(hWnd_tmp);
    int pixel_format_tmp = ChoosePixelFormat(hdc_tmp, &pfd_tmp);

    SetPixelFormat(hdc_tmp, pixel_format_tmp, &pfd_tmp);

    HGLRC gl_context_tmp = wglCreateContext(hdc_tmp);
    wglMakeCurrent(hdc_tmp, gl_context_tmp);

    WGLEXTLoadFunctions();

    // Proper context creation
    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(wc);
    if (tooling_gui_enabled) {
        wc.lpfnWndProc = WndProcToolGui;
    } else {
        wc.lpfnWndProc = WndProc;
    }
    wc.hInstance = GetModuleHandle(0);
    //wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "MainWindow";
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(GetModuleHandle(0), "IDI_ICON1");
    if (!RegisterClassExA(&wc)) {
        return 1;
    }

    DWORD style = WS_OVERLAPPEDWINDOW;
    if (tooling_gui_enabled) {
        style = WS_OVERLAPPEDWINDOW;
    }
    RECT wr = { 0, 0, s_window_width, s_window_height };
    AdjustWindowRect(&wr, style, FALSE);

    if (show_window) {
        style |= WS_VISIBLE;
    }
    s_hWnd = CreateWindow(wc.lpszClassName, "Omega", style, CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top, 0, 0, wc.hInstance, 0);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    HDC hdc = GetDC(s_hWnd);
    const int attribList[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_ALPHA_BITS_ARB, 8,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
        WGL_SAMPLES_ARB, 4,
        0
    };
    UINT num_formats = 0;
    int pixel_format = 0;
    wglChoosePixelFormatARB(hdc, attribList, 0, 1, &pixel_format, &num_formats);

    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(hdc, pixel_format, sizeof(pfd), &pfd);
    SetPixelFormat(hdc, pixel_format, &pfd);

    int contextAttribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 6,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };
    s_gl_context = wglCreateContextAttribsARB(hdc, 0, contextAttribs);

    // Destroy temporary context and window
    wglMakeCurrent(0, 0);
    wglDeleteContext(gl_context_tmp);
    ReleaseDC(hWnd_tmp, hdc_tmp);
    DestroyWindow(hWnd_tmp);

    //
    wglMakeCurrent(hdc, s_gl_context);
    GLEXTLoadFunctions();
    s_hdc = hdc;

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniformBufferBindings);
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxTextureBufferSz);

    LOG("GL VERSION: " << (char*)glGetString(GL_VERSION));
    LOG("GL_MAX_VERTEX_ATTRIBS: " << maxAttribs);
    LOG("GL_MAX_UNIFORM_BUFFER_BINDINGS: " << maxUniformBufferBindings);
    LOG("GL_MAX_TEXTURE_BUFFER_SIZE: " << maxTextureBufferSz);

#ifdef _DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(&glDbgCallback, 0);
#endif

    // let the gpu rest
    wglSwapIntervalEXT(1);

    // Raw input
    RAWINPUTDEVICE rid[2];
    rid[0].usUsagePage = 0x01;
    rid[0].usUsage = 0x02;
    rid[0].dwFlags = 0;
    rid[0].hwndTarget = 0;
    rid[1].usUsagePage = 0x01;
    rid[1].usUsage = 0x06;
    rid[1].dwFlags = 0;
    rid[1].hwndTarget = 0;
    if (RegisterRawInputDevices(rid, 2, sizeof(rid[0])) == FALSE) {
        LOG_ERR("RegisterRawInputDevices failed!");
        assert(false);
    }

    if (tooling_gui_enabled) {
        DragAcceptFiles(s_hWnd, TRUE);
    }

    return 0;
}
void platformCleanup() {

}

bool platformIsRunning() {
    return s_is_running;
}
void platformPollMessages() {
    MSG msg = { 0 };
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        if (msg.message == WM_QUIT) {
            s_is_running = false;
        }
    }
}

void platformSwapBuffers() {
    SwapBuffers(s_hdc);
}
void platformRestoreContext() {
    wglMakeCurrent(s_hdc, s_gl_context);
}

void platformGetWindowSize(int& w, int &h) {
    w = s_window_width;
    h = s_window_height;
}

const gfxm::rect& platformGetViewportRect() {
    int w = 0, h = 0;
    platformGetWindowSize(w, h);
    s_viewport_rect = gfxm::rect(gfxm::vec2(.0f, .0f), gfxm::vec2((float)w, (float)h));
    return s_viewport_rect;
}

void platformSetWindowResizeCallback(platform_window_resize_cb_t cb) {
    s_window_resize_cb_f = cb;
}


static float s_mouse_x = 0, s_mouse_y = 0;
void platformGetMousePos(int* x, int* y) {
    *x = (int)s_mouse_x;
    *y = (int)s_mouse_y;
}

struct MouseState {
    bool lock;
    bool hide;
};
static std::stack<MouseState> mouse_state_stack;
void platformPushMouseState(bool lock, bool hide) {
    MouseState state{ lock, hide };
    platformLockMouse(lock);
    platformHideMouse(hide);
    mouse_state_stack.push(state);
}
void platformPopMouseState() {
    if (mouse_state_stack.empty()) {
        assert(false);
        return;
    }
    mouse_state_stack.pop();
    
    if (mouse_state_stack.empty()) {
        platformLockMouse(false);
        platformHideMouse(false);
    } else {
        MouseState state = mouse_state_stack.top();
        platformLockMouse(state.lock);
        platformHideMouse(state.hide);
    }
}
void platformLockMouse(bool lock) {
    s_is_mouse_locked = lock;
    if (lock && GetActiveWindow() == s_hWnd) {
        RECT rc;
        GetWindowRect(s_hWnd, &rc);
        rc.left = rc.right = rc.left + (rc.right - rc.left) * .5f;
        rc.top = rc.bottom = rc.top + (rc.bottom - rc.top) * .5f;
        // TODO: Update clip area when window changes size
        ClipCursor(&rc);
    } else {
        ClipCursor(0);
    }
}
void platformHideMouse(bool hide) {
    s_is_mouse_hidden = hide;
    if (hide && GetActiveWindow() == s_hWnd) {
        ShowCursor(false);
    } else {
        ShowCursor(true);
    }
}
void platfromCaptureMouse() {
    SetCapture(s_hWnd);
}
void platformReleaseMouse() {
    ReleaseCapture();
}

int platformGeti(PLATFORM_PARAM param) {
    switch (param) {
    case PLATFORM_MAX_ATTRIB_COUNT:
        return maxAttribs;
    case PLATFORM_MAX_UNIFORM_BUFFER_BINDINGS:
        return maxUniformBufferBindings;
    case PLATFORM_MAX_TEXTURE_BUFFER_SZ:
        return maxTextureBufferSz;
    default:
        return -1;
    }
}

static void toggleFullscreen() {
    s_is_fullscreen = !s_is_fullscreen;

    if (!s_is_fullscreen) {
        RECT monitor_rc = { 0 };
        {
            HMONITOR hMon = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFOEX minf = { 0 };
            minf.cbSize = sizeof(minf);
            if (!GetMonitorInfoA(hMon, &minf)) {
                LOG_ERR("win32", "Failed to get monitor info");
                return;
            }
            monitor_rc = minf.rcMonitor;
        }

        s_window_width = s_windowed_width;
        s_window_height = s_windowed_height;

        DWORD style = WS_OVERLAPPEDWINDOW;
        RECT wr = { 0, 0, s_window_width, s_window_height };
        AdjustWindowRect(&wr, style, FALSE);

        SetWindowLongPtr(s_hWnd, GWL_STYLE, style);
        SetWindowPlacement(s_hWnd, &s_window_placement);
    } else {
        GetWindowPlacement(s_hWnd, &s_window_placement);

        RECT rc = { 0 };
        GetWindowRect(s_hWnd, &rc);
        HMONITOR hMon = MonitorFromRect(&rc, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFOEX minf = { 0 };
        minf.cbSize = sizeof(minf);
        if (!GetMonitorInfoA(hMon, &minf)) {
            LOG_ERR("win32", "Failed to get monitor info");
            return;
        }
        rc = minf.rcMonitor;
        LOG("win32", "Going fullscreen to monitor " << minf.szDevice);
        LOG("win32", "Rectangle left: " << rc.left << " top: " << rc.top << " right: " << rc.right << " bottom: " << rc.bottom);

        SetWindowLongPtr(s_hWnd, GWL_STYLE, WS_POPUP);
        SetWindowPos(s_hWnd, 0/*HWND_TOPMOST*/, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_SHOWWINDOW);
    }
}

#include <windowsx.h>
#include "input/input.hpp"
#include "gui/gui.hpp"
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_MOUSEACTIVATE:
        return MA_ACTIVATEANDEAT;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE) {
            if (s_is_mouse_locked) {
                RECT rc;
                GetWindowRect(s_hWnd, &rc);
                rc.left = rc.right = rc.left + (rc.right - rc.left) * .5f;
                rc.top = rc.bottom = rc.top + (rc.bottom - rc.top) * .5f;
                ClipCursor(&rc);
            }
            if (s_is_mouse_hidden) {
                ShowCursor(false);
            }
        } else if(LOWORD(wParam) == WA_INACTIVE) {
            if (s_is_mouse_hidden) {
                ShowCursor(true);
            }
        }
        break;
    case WM_SIZE: {
        RECT wr = { 0 };
        GetClientRect(hWnd, &wr);
        s_window_width = wr.right - wr.left;
        s_window_height = wr.bottom - wr.top;
        if (!s_is_fullscreen) {
            s_windowed_width = s_window_width;
            s_windowed_height = s_window_height;
        }
        if (s_window_resize_cb_f) {
            s_window_resize_cb_f(s_window_width, s_window_height);
        }
        } break;
    case WM_KEYDOWN:
        //inputPost(InputDeviceType::Keyboard, 0, wParam, 1.0f);
        break;
    case WM_KEYUP:
        //inputPost(InputDeviceType::Keyboard, 0, wParam, 0.0f);
        break;    
    case WM_LBUTTONDOWN:
        if (guiIsMouseCaptured()) {
            guiPostMessage(GUI_MSG::LBUTTON_DOWN); // Check if gui actually processed it, otherwise send to the input system
        } else {
            inputPost(InputDeviceType::Mouse, 0, Key.Mouse.BtnLeft, 1.0f);
        }
        break;
    case WM_LBUTTONUP:
        if (guiIsMouseCaptured()) {
            guiPostMessage(GUI_MSG::LBUTTON_UP);
        } else {
            inputPost(InputDeviceType::Mouse, 0, Key.Mouse.BtnLeft, 0.0f);
        }
        break;
    case WM_RBUTTONDOWN:
        if (guiIsMouseCaptured()) {
            guiPostMessage(GUI_MSG::RBUTTON_DOWN);
        } else {
            inputPost(InputDeviceType::Mouse, 0, Key.Mouse.BtnRight, 1.0f);
        }
        break;
    case WM_RBUTTONUP:
        if (guiIsMouseCaptured()) {
            guiPostMessage(GUI_MSG::RBUTTON_UP);
        } else {
            inputPost(InputDeviceType::Mouse, 0, Key.Mouse.BtnRight, 0.0f);
        }
        break;
    case WM_MBUTTONDOWN:
        if (guiIsMouseCaptured()) {
            guiPostMessage(GUI_MSG::MBUTTON_DOWN);
        } else {
            inputPost(InputDeviceType::Mouse, 0, Key.Mouse.Btn3, 1.0f);
        }
        break;
    case WM_MBUTTONUP:
        if (guiIsMouseCaptured()) {
            guiPostMessage(GUI_MSG::MBUTTON_UP);
        } else {
            inputPost(InputDeviceType::Mouse, 0, Key.Mouse.Btn3, 0.0f);
        }
        break;
    case WM_MOUSEWHEEL:
        if (guiIsMouseCaptured()) {
            guiPostMessage<int32_t, int>(GUI_MSG::MOUSE_SCROLL, GET_WHEEL_DELTA_WPARAM(wParam), 0);
        } else {
            inputPost(InputDeviceType::Mouse, 0, Key.Mouse.Scroll, GET_WHEEL_DELTA_WPARAM(wParam) / 120, InputKeyType::Increment);
        }
        break;
    case WM_MOUSEMOVE:/*
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.AxisX, GET_X_LPARAM(lParam), InputKeyType::Absolute);
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.AxisY, GET_Y_LPARAM(lParam), InputKeyType::Absolute);*/
        guiPostMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        s_mouse_x = GET_X_LPARAM(lParam);
        s_mouse_y = GET_Y_LPARAM(lParam);
        break;
    case WM_CHAR:
        guiPostMessage<uint32_t, uint32_t>(GUI_MSG::UNICHAR, wParam, 0); // TODO
        break;
    case WM_INPUT: {
        UINT dwSize;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
        LPBYTE lpb = new BYTE[dwSize];
        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
            LOG_ERR(":|");
        }
        RAWINPUT* raw = (RAWINPUT*)lpb;
        if (raw->header.dwType == RIM_TYPEMOUSE) {
            if (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {

            } else {
                // TODO: ?
            }
            inputPost(InputDeviceType::Mouse, 0, Key.Mouse.AxisX, -raw->data.mouse.lLastX, InputKeyType::Increment);
            inputPost(InputDeviceType::Mouse, 0, Key.Mouse.AxisY, -raw->data.mouse.lLastY, InputKeyType::Increment);
        } else if(raw->header.dwType == RIM_TYPEKEYBOARD) {
            RAWKEYBOARD& rk = raw->data.keyboard;
            USHORT vk = rk.VKey;
            uint16_t gui_vk = rk.VKey;
            if (vk == VK_SHIFT) {
                if (rk.MakeCode == 0x2a) {
                    vk = Key.Keyboard.LeftShift;
                } else if(rk.MakeCode == 0x36) {
                    vk = Key.Keyboard.RightShift;
                }
            }
            if((rk.Flags & RI_KEY_BREAK) == RI_KEY_BREAK) {
                inputPost(InputDeviceType::Keyboard, 0, vk, 0.0f);
                guiPostMessage<uint16_t, int>(GUI_MSG::KEYUP, gui_vk, 0);
            } else {
                inputPost(InputDeviceType::Keyboard, 0, vk, 1.0f);
                guiPostMessage<uint16_t, int>(GUI_MSG::KEYDOWN, gui_vk, 0);
            }
        }
        } break;
    case WM_SYSCOMMAND: {
        if (SC_KEYMENU == (wParam & 0xFFF0) && lParam == VK_RETURN) {
            toggleFullscreen();
        } else {
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
        break;
    }
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK WndProcToolGui(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE) {
            if (s_is_mouse_locked) {
                RECT rc;
                GetWindowRect(s_hWnd, &rc);
                rc.left = rc.right = rc.left + (rc.right - rc.left) * .5f;
                rc.top = rc.bottom = rc.top + (rc.bottom - rc.top) * .5f;
                ClipCursor(&rc);
            }
            if (s_is_mouse_hidden) {
                ShowCursor(false);
            }
        } else if(LOWORD(wParam) == WA_INACTIVE) {
            if (s_is_mouse_hidden) {
                ShowCursor(true);
            }
        }
        break;
    case WM_SIZE: {
        RECT wr = { 0 };
        GetClientRect(hWnd, &wr);
        s_window_width = wr.right - wr.left;
        s_window_height = wr.bottom - wr.top;
        if (s_window_resize_cb_f) {
            s_window_resize_cb_f(s_window_width, s_window_height);
        }
        } break;
    case WM_KEYDOWN:
        //guiLayout();
        //inputPost(InputDeviceType::Keyboard, 0, wParam, 1.0f);
        //guiPostMessage(GUI_MSG::KEYDOWN, wParam, 0);
        break;
    case WM_KEYUP:
        //inputPost(InputDeviceType::Keyboard, 0, wParam, 0.0f);
        //guiPostMessage(GUI_MSG::KEYUP, wParam, 0);
        break;
    case WM_LBUTTONDOWN:
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.BtnLeft, 1.0f);
        guiPostMessage(GUI_MSG::LBUTTON_DOWN); // Check if gui actually processed it, otherwise send to the input system
        break;
    case WM_LBUTTONUP:
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.BtnLeft, 0.0f);
        guiPostMessage(GUI_MSG::LBUTTON_UP);
        break;
    case WM_RBUTTONDOWN:
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.BtnRight, 1.0f);
        guiPostMessage(GUI_MSG::RBUTTON_DOWN);
        break;
    case WM_RBUTTONUP:
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.BtnRight, 0.0f);
        guiPostMessage(GUI_MSG::RBUTTON_UP);
        break;
    case WM_MBUTTONDOWN:
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.Btn3, 1.0f);
        guiPostMessage(GUI_MSG::MBUTTON_DOWN);
        break;
    case WM_MBUTTONUP:
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.Btn3, 0.0f);
        guiPostMessage(GUI_MSG::MBUTTON_UP);
        break;
    case WM_MOUSEWHEEL:
        guiPostMessage<int32_t, int>(GUI_MSG::MOUSE_SCROLL, GET_WHEEL_DELTA_WPARAM(wParam), 0);
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.Scroll, GET_WHEEL_DELTA_WPARAM(wParam) / 120, InputKeyType::Increment);
        break;
    case WM_MOUSEMOVE:/*
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.AxisX, GET_X_LPARAM(lParam), InputKeyType::Absolute);
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.AxisY, GET_Y_LPARAM(lParam), InputKeyType::Absolute);*/
        guiPostMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        s_mouse_x = GET_X_LPARAM(lParam);
        s_mouse_y = GET_Y_LPARAM(lParam);
        break;
    case WM_CHAR:
        guiPostMessage<uint32_t, uint32_t>(GUI_MSG::UNICHAR, wParam, 0); // TODO
        break;
    case WM_SETCURSOR:
        return 0;
        break;
    case WM_INPUT: {
        UINT dwSize;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
        LPBYTE lpb = new BYTE[dwSize];
        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
            LOG_ERR(":|");
        }
        RAWINPUT* raw = (RAWINPUT*)lpb;
        if (raw->header.dwType == RIM_TYPEMOUSE) {
            if (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {

            } else {
                // TODO: ?
            }
            inputPost(InputDeviceType::Mouse, 0, Key.Mouse.AxisX, -raw->data.mouse.lLastX, InputKeyType::Increment);
            inputPost(InputDeviceType::Mouse, 0, Key.Mouse.AxisY, -raw->data.mouse.lLastY, InputKeyType::Increment);
        } else if(raw->header.dwType == RIM_TYPEKEYBOARD) {
            RAWKEYBOARD& rk = raw->data.keyboard;
            uint16_t vk = rk.VKey;
            uint16_t gui_vk = rk.VKey;
            if (vk == VK_SHIFT) {
                if (rk.MakeCode == 0x2a) {
                    vk = Key.Keyboard.LeftShift;
                } else if(rk.MakeCode == 0x36) {
                    vk = Key.Keyboard.RightShift;
                }
            }
            if((rk.Flags & RI_KEY_BREAK) == RI_KEY_BREAK) {
                inputPost(InputDeviceType::Keyboard, 0, vk, 0.0f);
                guiPostMessage<uint16_t, int>(GUI_MSG::KEYUP, gui_vk, 0);
            } else {
                inputPost(InputDeviceType::Keyboard, 0, vk, 1.0f);
                guiPostMessage<uint16_t, int>(GUI_MSG::KEYDOWN, gui_vk, 0);
            }
        }
        } break;
    case WM_DROPFILES: {
        HDROP hDrop = (HDROP)wParam;
        int file_count = DragQueryFileW(hDrop, 0xFFFFFFFF, 0, 0);
        POINT p; // mouse pos at the time of the drop
        DragQueryPoint(hDrop, &p);
        gfxm::vec2 pt(p.x, p.y);
        for (int i = 0; i < file_count; ++i) {
            std::wstring wstr;
            int name_len = DragQueryFileW(hDrop, i, 0, 0);
            if (name_len == 0) {
                assert(false);
                continue;
            }
            wstr.resize(name_len + 1);
            DragQueryFileW(hDrop, i, (LPWSTR)wstr.data(), name_len + 1);
            
            std::filesystem::path path(wstr);
            guiPostDropFile(pt, path);
        }
        DragFinish(hDrop);
        }break;
    case WM_SYSCOMMAND: {
        if (SC_KEYMENU == (wParam & 0xFFF0) && lParam == VK_RETURN) {
            toggleFullscreen();
        } else {
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
        break;
    }
    
    // This results in an undesirable hit test that causes bugs with viewport gizmos
    /*case WM_NCHITTEST: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        RECT rect = { 0 };
        GetWindowRect(s_hWnd, &rect);
        x -= rect.left;
        y -= rect.top;
        //int x = (int)guiGetMousePos().x;
        //int y = (int)guiGetMousePos().y;
        //printf(std::format("{}, {}\n", x, y).c_str());
        GuiHitResult hit;
        guiGetRoot()->hitTest(hit, x, y);
        if (hit.hasHit()) {
            for (auto& h : hit.hits) {
                switch (h.hit) {
                case GUI_HIT::NATIVE_CAPTION:
                    return HTCAPTION;
                }
            }
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }*/
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}