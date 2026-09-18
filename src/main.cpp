#include "core/Application.h"
#include "core/Types/Settings.h"
#include "core/AssetManager.h"
#include "core/LindoCrashHandle.h"

#include <array>
#include <string>
#include <iostream>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <unknwn.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

// Если нужно гарантированно скрыть консоль при запускe в MSVC/Windows без изменения CMake
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

using namespace Gdiplus;

namespace {
    // Процедура окна Splash Screen (заставки)
    LRESULT CALLBACK SplashWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        static Image* bannerImg = nullptr;
        switch (msg) {
        case WM_CREATE: {
            std::string fullPath = Lindo::AssetManager::get().resolvePath("textures/banner.png");
            std::wstring wFullPath(fullPath.begin(), fullPath.end());

            bannerImg = Image::FromFile(wFullPath.c_str());
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            Graphics graphics(hdc);

            if (bannerImg && bannerImg->GetLastStatus() == Ok) {
                graphics.DrawImage(bannerImg, 0, 0, bannerImg->GetWidth(), bannerImg->GetHeight());
            }
            else {
                SolidBrush brush(Color(255, 30, 30, 30));
                graphics.FillRectangle(&brush, 0, 0, 600, 350);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            if (bannerImg) delete bannerImg;
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    // Фоновый поток для отображения заставки во время загрузки
    void RunSplashThread() {
        HANDLE hCloseEvent = CreateEventW(NULL, TRUE, FALSE, L"Global\\LindoSplash");

        GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

        const wchar_t CLASS_NAME[] = L"LindoSplash";
        WNDCLASSW wc = {};
        wc.lpfnWndProc = SplashWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);

        RegisterClassW(&wc);

        int width = 600;
        int height = 350;
        int posX = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
        int posY = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

        HWND hwnd = CreateWindowExW(
            WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
            CLASS_NAME, L"Lindo Loading",
            WS_POPUP | WS_VISIBLE,
            posX, posY, width, height,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        MSG msg = {};
        bool shouldClose = false;

        while (!shouldClose) {
            while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (hCloseEvent && WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0) {
                shouldClose = true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        DestroyWindow(hwnd);
        UnregisterClassW(CLASS_NAME, GetModuleHandle(NULL));
        GdiplusShutdown(gdiplusToken);

        if (hCloseEvent) {
            CloseHandle(hCloseEvent);
        }
    }
}
#endif

int main() {
    Lindo::Core::LindoCrashHandle::SetApplicationName(Lindo::AppInfo::Name);
    Lindo::Core::LindoCrashHandle::Install();

#ifdef _WIN32
    // Запускаем окно заставки без консольного окна
    std::thread splashThread(RunSplashThread);
#endif

    try {
        Lindo::DisplaySettings::getInstance().loadFromFile("../config/display.ini");
        Lindo::Settings::getInstance().loadFromFile("../config/settings.ini");

        Lindo::Application app;
        app.run();
    }
    catch (const std::exception& e) {
        Lindo::Core::LindoCrashHandle::ReportException("main exception", e.what());
        std::cerr << "Fatal error: " << e.what() << std::endl;
#ifdef _WIN32
        HANDLE hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, L"Global\\LindoSplash");
        if (hEvent) {
            SetEvent(hEvent);
            CloseHandle(hEvent);
        }
        if (splashThread.joinable()) splashThread.join();
#endif
        return -1;
    }
    catch (...) {
        Lindo::Core::LindoCrashHandle::ReportException("main exception", "unknown exception");
#ifdef _WIN32
        HANDLE hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, L"Global\\LindoSplash");
        if (hEvent) {
            SetEvent(hEvent);
            CloseHandle(hEvent);
        }
        if (splashThread.joinable()) splashThread.join();
#endif
        return -1;
    }

#ifdef _WIN32
    if (splashThread.joinable()) splashThread.join();
#endif

    return 0;
}