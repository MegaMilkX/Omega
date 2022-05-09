#include "platform.hpp"

#include <Windows.h>
#include <gl/GL.h>

#include "platform/gl/glextutil.h"
#include "common/log/log.hpp"

static bool s_is_running = true;

static HDC s_hdc = 0;

static int s_window_width = 1920, s_window_height = 1080;
static gfxm::rect s_viewport_rect(gfxm::vec2(0, 0), gfxm::vec2(1920, 1080));
static platform_window_resize_cb_t s_window_resize_cb_f = 0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int platformInit() {
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
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(0);
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    wc.lpszClassName = "MainWindow";
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(GetModuleHandle(0), "IDI_ICON1");
    if (!RegisterClassExA(&wc)) {
        return 1;
    }

    RECT wr = { 0, 0, s_window_width, s_window_height };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    HWND hWnd = CreateWindow(wc.lpszClassName, "Omega", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, wr.right - wr.left, wr.bottom - wr.top, 0, 0, wc.hInstance, 0);
    HDC hdc = GetDC(hWnd);
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
    HGLRC gl_context = wglCreateContextAttribsARB(hdc, 0, contextAttribs);

    // Destroy temporary context and window
    wglMakeCurrent(0, 0);
    wglDeleteContext(gl_context_tmp);
    ReleaseDC(hWnd_tmp, hdc_tmp);
    DestroyWindow(hWnd_tmp);

    //
    wglMakeCurrent(hdc, gl_context);
    GLEXTLoadFunctions();
    s_hdc = hdc;

    GLint maxAttribs = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
    GLint maxUniformBufferBindings = 0;
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniformBufferBindings);

    LOG("GL VERSION: " << (char*)glGetString(GL_VERSION));
    LOG("GL_MAX_VERTEX_ATTRIBS: " << maxAttribs);
    LOG("GL_MAX_UNIFORM_BUFFER_BINDINGS: " << maxUniformBufferBindings);
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


static int s_mouse_x = 0, s_mouse_y = 0;
void platformGetMousePos(int* x, int* y) {
    *x = s_mouse_x;
    *y = s_mouse_y;
}

#include <windowsx.h>
#include "common/input/input.hpp"
#include "common/gui/gui.hpp"
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
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
        inputPost(InputDeviceType::Keyboard, 0, wParam, 1.0f);
        break;
    case WM_KEYUP:
        inputPost(InputDeviceType::Keyboard, 0, wParam, 0.0f);
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
        break;
    case WM_RBUTTONUP:
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.BtnRight, 0.0f);
        break;
    case WM_MBUTTONDOWN:
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.Btn3, 1.0f);
        break;
    case WM_MBUTTONUP:
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.Btn3, 0.0f);
        break;
    case WM_MOUSEMOVE:
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.AxisX, GET_X_LPARAM(lParam), InputKeyType::Absolute);
        inputPost(InputDeviceType::Mouse, 0, Key.Mouse.AxisY, GET_Y_LPARAM(lParam), InputKeyType::Absolute);
        guiPostMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        s_mouse_x = GET_X_LPARAM(lParam);
        s_mouse_y = GET_Y_LPARAM(lParam);
        break;
    case WM_SETCURSOR:
        return 0;
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}