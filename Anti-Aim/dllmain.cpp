#include <Windows.h>
#include <cstdio>
#include "menu.h"
#include "hooks.h"
#include <atomic>

extern std::atomic<int> g_createMoveCalls;
extern std::atomic<int> g_validateInputCalls;
extern std::atomic<int> g_onMoveRuns;

static volatile bool running = true;

DWORD WINAPI MainThread(LPVOID lpParam)
{
    HMODULE hModule = (HMODULE)lpParam;

    AllocConsole();
    FILE* f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
    printf("Waiting for client.dll...\n");

    uintptr_t clientBase = 0;
    while (!(clientBase = (uintptr_t)GetModuleHandleW(L"client.dll")))
        Sleep(500);

    printf("client.dll found, init ESP state...\n");
    InitEspState(clientBase);

    Sleep(2000);

    if (!Hooks::Init())
        printf("Hooks::Init failed\n");
    else
        printf("Hooks installed OK\n");

    HWND overlay = SetupOverlay();
    if (!overlay) {
        printf("Overlay setup failed\n");
        if (f) fclose(f);
        FreeConsole();
        FreeLibraryAndExitThread(hModule, 0);
        return 0;
    }
    printf("Overlay ready (F11 to unload)\n");

    MSG msg = {};
    ULONGLONG lastLog = 0;
    while (running && msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        if (GetAsyncKeyState(VK_F11) & 1)
            break;

        // ---- heartbeat: prints regardless of focus ----
        if (GetTickCount64() - lastLog > 2000) {
            printf("CM=%d VI=%d OM=%d\n",
                g_createMoveCalls.load(),
                g_validateInputCalls.load(),
                g_onMoveRuns.load());
            lastLog = GetTickCount64();
        }

        // ---- only draw the overlay when CS2 is focused ----
        HWND fg = GetForegroundWindow();
        char cls[256] = {};
        GetClassNameA(fg, cls, 256);
        if (strcmp(cls, "SDL_app") != 0) { Sleep(50); continue; }

        OverlayFrame();
        Sleep(1);
    }

    Hooks::Shutdown();
    TeardownOverlay();

    if (f) fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE t = CreateThread(nullptr, 0,
            (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr);
        if (t) CloseHandle(t);
    }
    else if (reason == DLL_PROCESS_DETACH) {
        running = false;
    }
    return TRUE;
}