#include <windows.h>
#include <GL/gl.h>
#include <math.h>

// Link native OpenGL
#pragma comment(lib, "opengl32.lib")

// Globals so WindowProc and main can share the GL context/HDC
static HDC g_hdc = NULL;
static HGLRC g_hglrc = NULL;
static HWND g_hwnd = NULL;

// 4 control points for a cubic Bézier curve
int controlPoints[4][2] = {
    {100, 100},
    {200, 400},
    {400, 400},
    {500, 100}
};

void initGL()
{
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glPointSize(6.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Simple orthographic projection matching window coordinates
    glOrtho(0.0, 600.0, 0.0, 600.0, -1.0, 1.0);
}

void drawDot(int x, int y)
{
    glBegin(GL_POINTS);
    glVertex2i(x, y);
    glEnd();
}

void drawLine(int x1, int y1, int x2, int y2)
{
    glBegin(GL_LINES);
    glVertex2i(x1, y1);
    glVertex2i(x2, y2);
    glEnd();
}

// Compute cubic Bernstein basis and point on curve (no factorials)
void computeBezierPoint(double t, double &x, double &y)
{
    double u = 1.0 - t;
    double b0 = u * u * u;            // (1-t)^3
    double b1 = 3.0 * t * u * u;      // 3t(1-t)^2
    double b2 = 3.0 * t * t * u;      // 3t^2(1-t)
    double b3 = t * t * t;            // t^3

    x = b0 * controlPoints[0][0] +
        b1 * controlPoints[1][0] +
        b2 * controlPoints[2][0] +
        b3 * controlPoints[3][0];

    y = b0 * controlPoints[0][1] +
        b1 * controlPoints[1][1] +
        b2 * controlPoints[2][1] +
        b3 * controlPoints[3][1];
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw control polygon (light gray)
    glColor3f(0.8f, 0.8f, 0.8f);
    for (int i = 0; i < 3; ++i)
        drawLine(controlPoints[i][0], controlPoints[i][1],
                 controlPoints[i + 1][0], controlPoints[i + 1][1]);

    // Draw control points (red)
    glColor3f(1.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i)
        drawDot(controlPoints[i][0], controlPoints[i][1]);

    // Draw Bézier curve (blue)
    glColor3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_LINE_STRIP);
    for (double t = 0.0; t <= 1.0001; t += 0.005) {
        double x, y;
        computeBezierPoint(t, x, y);
        glVertex2d(x, y);
    }
    glEnd();

    glFlush();
    SwapBuffers(g_hdc);
}

// Basic WindowProc
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

int main()
{
    // Register window class (ANSI version to avoid Unicode entry point complexity)
    WNDCLASSA wc = {};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "OpenGLSimpleClass";
    RegisterClassA(&wc);

    // Create window
    g_hwnd = CreateWindowExA(
        0, wc.lpszClassName, "Bezier Curve - OpenGL (no GLUT)",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 600, 600,
        NULL, NULL, wc.hInstance, NULL);

    if (!g_hwnd) return 1;

    // Setup pixel format for OpenGL
    g_hdc = GetDC(g_hwnd);
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pf = ChoosePixelFormat(g_hdc, &pfd);
    if (pf == 0) return 2;
    if (!SetPixelFormat(g_hdc, pf, &pfd)) return 3;

    // Create and activate GL context
    g_hglrc = wglCreateContext(g_hdc);
    if (!g_hglrc) return 4;
    if (!wglMakeCurrent(g_hdc, g_hglrc)) return 5;

    initGL();

    // Simple loop that renders continuously and processes Windows messages
    bool running = true;
    MSG msg;
    while (running) {
        // Process all pending messages
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        // Render frame
        render();

        // ~60 FPS throttle
        Sleep(16);
    }

    // Cleanup
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(g_hglrc);
    ReleaseDC(g_hwnd, g_hdc);
    DestroyWindow(g_hwnd);

    return 0;
}