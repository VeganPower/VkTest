#include <Windows.h>
#include <iostream>

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    /*if (vulkanExample != NULL)
    {
        vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
    }*/
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

HWND setup_window(HINSTANCE hinstance, UINT width, UINT height, const char* win_name)
{
    WNDCLASSEX wndClass;

    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hinstance;
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = win_name;
    wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

    if (!RegisterClassEx(&wndClass))
    {
        std::cout << "Could not register window class!\n";
        return 0;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    DWORD dwExStyle;
    DWORD dwStyle;

    dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    RECT windowRect;

    windowRect.left = (long)screenWidth / 2 - width / 2;
    windowRect.right = (long)width;
    windowRect.top = (long)screenHeight / 2 - height / 2;
    windowRect.bottom = (long)height;

    AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

    HWND window = CreateWindowEx(0,
		win_name,
		win_name,
        //      WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU,
        dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        windowRect.left,
        windowRect.top,
        windowRect.right,
        windowRect.bottom,
        NULL,
        NULL,
        hinstance,
        NULL);

    if (!window)
    {
        std::cout << "Could not create window!\n";
        return 0;
    }

    ShowWindow(window, SW_SHOW);
    SetForegroundWindow(window);
    SetFocus(window);

    return window;
}