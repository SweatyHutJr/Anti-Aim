#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <array>
#include <vector>
#include <unordered_map>

#include "ImGUI/imgui.h"
#include "ImGUI/backends/imgui_impl_win32.h"
#include "ImGui/backends/imgui_impl_dx11.h"

#include "client_dll.hpp"
#include "offsets.hpp"
#include "buttons.hpp"
#include "bhop.h"
#include "WorldToScreen.h"
#include "BoneBoxESP.h"
#include "Triggerbot.h"
#include "Aimbot.h"
#include "buttons.hpp"
#include "recoil.h"
#include "hooks.h"
#include "AntiAim.h"

inline void DrawRadar();   // defined in Radar.h
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// ============================================================================
// D3D globals
// ============================================================================
inline ID3D11Device* pd3dDevice = nullptr;
inline ID3D11DeviceContext* pd3dDeviceContext = nullptr;
inline IDXGISwapChain* swapChain = nullptr;
inline ID3D11RenderTargetView* mainRenderTargetView = nullptr;
inline HWND                     hwndOverlay = nullptr;

// ============================================================================
// ESP state
// ============================================================================
inline uintptr_t clientBase = 0;
inline uintptr_t entityList = 0;
inline uintptr_t viewMatrixPtr = 0;

inline std::unordered_map<uintptr_t, uintptr_t>  boneArrayCache;
inline std::unordered_map<uintptr_t, PlayerMeta> metaCache;
inline std::vector<uintptr_t>                    cachedControllers;
inline std::vector<uintptr_t>                    cachedPawns;
inline ULONGLONG                                 lastCacheTime = 0;

inline void InitEspState(uintptr_t base) {
    clientBase = base;
    entityList = read<uintptr_t>(base + cs2_dumper::offsets::client_dll::dwEntityList);
    viewMatrixPtr = base + cs2_dumper::offsets::client_dll::dwViewMatrix;
}

inline void RefreshPlayerCache(uintptr_t localPawn) {
    uintptr_t listEntry = read<uintptr_t>(entityList + 0x10);
    if (!listEntry) return;
    if (GetTickCount64() - lastCacheTime <= 3000 && !cachedControllers.empty()) return;

    cachedControllers.clear();
    cachedPawns.clear();

    for (int i = 1; i < 64; i++) {
        uintptr_t pc = read<uintptr_t>(listEntry + i * 0x70);
        if (!pc) continue;
        uintptr_t pawnId = read<uintptr_t>(pc + cs2_dumper::schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
        if (!pawnId) continue;
        uintptr_t le2 = read<uintptr_t>(entityList + 0x8 * ((pawnId & 0x7FFF) >> 9) + 0x10);
        if (!le2) continue;
        uintptr_t pawn = read<uintptr_t>(le2 + 0x70 * (pawnId & 0x1FF));
        if (!pawn) continue;

        cachedControllers.push_back(pc);
        cachedPawns.push_back(pawn);

        PlayerMeta meta;
        meta.team = read<int>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
        nameBuf nb = read<nameBuf>(pc + cs2_dumper::schemas::client_dll::CBasePlayerController::m_iszPlayerName);
        memcpy(meta.name, nb.data, sizeof(meta.name));
        meta.nameValid = true;
        metaCache[pawn] = meta;
    }

    auto prune = [&](auto& m) {
        for (auto it = m.begin(); it != m.end(); )
            if (std::find(cachedPawns.begin(), cachedPawns.end(), it->first) == cachedPawns.end())
                it = m.erase(it);
            else ++it;
        };
    prune(boneArrayCache);
    prune(metaCache);

    lastCacheTime = GetTickCount64();
}

// ============================================================================
// ImGui cheat menu
// ============================================================================
inline void DrawMenu() {
    static bool showMenu = false;
    static bool lastShow = false;

    if (GetAsyncKeyState(VK_INSERT) & 1)
        showMenu = !showMenu;

    if (showMenu != lastShow && hwndOverlay) {
        LONG_PTR ex = GetWindowLongPtrW(hwndOverlay, GWL_EXSTYLE);
        if (showMenu) ex &= ~WS_EX_TRANSPARENT;
        else          ex |= WS_EX_TRANSPARENT;
        SetWindowLongPtrW(hwndOverlay, GWL_EXSTYLE, ex);
        lastShow = showMenu;
    }

    if (!showMenu) return;

    ImGui::Begin("Cheat Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Checkbox("ESP Enabled", &settings.espEnabled);
    ImGui::Checkbox("Team Check", &settings.teamCheck);

    if (ImGui::CollapsingHeader("ESP Elements", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Box", &settings.showBox);
        ImGui::Checkbox("Names", &settings.showNames);
        ImGui::Checkbox("Distance", &settings.showDistance);
        ImGui::Checkbox("Bones", &settings.showBones);
        ImGui::Checkbox("Health Bar", &settings.showHealth);
    }

    ImGui::Separator();

    ImGui::Checkbox("Aimbot", &settings.aimbotEnabled);
    ImGui::Checkbox("Aimbot Visible Only", &settings.aimbotVisibleOnly);
    ImGui::Checkbox("Aimbot Target Teammates", &settings.aimbotTargetTeammates);
    ImGui::Checkbox("Show FOV Circle", &settings.showFovCircle);
    ImGui::SliderFloat("Aimbot FOV", &settings.aimbotFov, 1.0f, 45.0f, "%.1f deg");
    ImGui::SliderFloat("Aimbot Smooth", &settings.aimbotSmooth, 1.0f, 50.0f, "%.1f");

    ImGui::Separator();

    ImGui::Checkbox("Triggerbot", &settings.triggerbotEnabled);
    ImGui::SliderFloat("Trigger Delay (ms)", &settings.triggerDelay, 0.0f, 1000.0f, "%.0f ms");
    ImGui::Separator();
    ImGui::Text("Recoil Control");
    ImGui::Checkbox("RCS Enabled", &settings.rcsEnabled);
    ImGui::SliderFloat("RCS Strength", &settings.rcsScale, 0.0f, 1.0f, "%.2f");
    ImGui::Checkbox("Bunnyhop", &settings.bhopEnabled);

    ImGui::Separator();
    ImGui::Text("Radar");
    ImGui::Checkbox("Radar Enabled", &settings.radarEnabled);
    ImGui::Checkbox("Radar Team Check", &settings.radarTeamCheck);
    ImGui::SliderFloat("Radar Range", &settings.radarRange, 500.0f, 8000.0f, "%.0f u");
    ImGui::SliderFloat("Radar Size", &settings.radarSize, 80.0f, 400.0f, "%.0f px");
    ImGui::SliderFloat("Radar Opacity", &settings.radarOpacity, 5.0f, 0.0f, "%.2f");
    ImGui::SliderFloat("Radar Offset X", &settings.radarOffsetX, 0.0f, 800.0f, "%.0f px");
    ImGui::SliderFloat("Radar Offset Y", &settings.radarOffsetY, 0.0f, 800.0f, "%.0f px");

    ImGui::Separator();
    ImGui::Text("Anti-Aim");
    ImGui::Checkbox("Anti-Aim Enabled", &settings.antiaimEnabled);
    ImGui::Combo("Mode", &settings.antiaimMode, "Jitter\0Spin\0Static\0Turn Around\0");

    switch (settings.antiaimMode) {
    case 0:
        ImGui::SliderFloat("Jitter Angle", &settings.antiaimJitterAngle, 0.f, 90.f, "%.0f deg");
        break;
    case 1:
        ImGui::SliderFloat("Spin Speed", &settings.antiaimSpinSpeed, 1.f, 20.f, "%.0f deg/tick");
        break;
    case 2:
        ImGui::SliderFloat("Yaw Offset", &settings.antiaimYaw, 0.f, 180.f, "%.0f deg");
        break;
    case 3:
        ImGui::TextDisabled("(no params)");
        break;
    }

    ImGui::SliderFloat("Pitch", &settings.antiaimPitch, -89.f, 89.f, "%.0f deg");
    ImGui::Checkbox("Movement Fix", &settings.antiaimMoveFix);

    ImGui::Separator();

    if (ImGui::Button("Reset defaults")) {
        settings.espEnabled = true;
        settings.teamCheck = true;
        settings.aimbotEnabled = false;
        settings.triggerbotEnabled = false;
        settings.bhopEnabled = true;
        settings.aimbotFov = 10.0f; 
        settings.aimbotSmooth = 1.0f;
        settings.showBox = true;
        settings.showNames = false;
        settings.showDistance = false;
        settings.showBones = false;
        settings.showHealth = true;
        settings.showFovCircle = false;
        settings.aimbotVisibleOnly = false;
        settings.aimbotTargetTeammates = false;
        settings.triggerDelay = 0.0f;
        
        settings.radarEnabled = false;
        settings.radarTeamCheck = true;
        settings.radarRange = 3000.0f;
        settings.radarSize = 150.0f;
        settings.radarOpacity = 0.70f;
        settings.radarOffsetX = 0.0f;
        settings.radarOffsetY = 0.0f;

        // Anti-aim
        bool  antiaimEnabled = true;
        bool  antiaimJitter = true;
        float antiaimPitch = 89.f;
        float antiaimYaw = 179.f;
    }

    ImGui::End();
}

inline void DrawESP(IDXGISwapChain* sc) {
    if (!sc || !clientBase) return;

    DXGI_SWAP_CHAIN_DESC sd = {};
    sc->GetDesc(&sd);
    const int W = (int)sd.BufferDesc.Width;
    const int H = (int)sd.BufferDesc.Height;
    if (W <= 0 || H <= 0) return;

    auto draw_list = ImGui::GetBackgroundDrawList();
    if (!draw_list) return;

    uintptr_t localPawn = read<uintptr_t>(clientBase + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;

    std::array<float, 16> viewMatrix{};
    if (!read_raw(viewMatrixPtr, viewMatrix.data(), sizeof(viewMatrix))) return;

    int localHealth = read<int>(localPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);
    int lTeam = read<int>(localPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);

    RunBhop(localPawn, clientBase);
    RefreshPlayerCache(localPawn);

    if (settings.rcsEnabled) {
        RunRCS(clientBase, localPawn);
    }

    oldOrigin localPos = read<oldOrigin>(localPawn + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin);
    Vector3 localEye = { localPos.cords[0], localPos.cords[1], localPos.cords[2] + 64.0f };

    const int centerX = W / 2;
    const int centerY = H / 2;

    if (settings.aimbotEnabled && settings.showFovCircle) {
        float r = (H / 2.0f) * tanf(settings.aimbotFov * (3.14159265359f / 180.0f));
        draw_list->AddCircle(ImVec2((float)centerX, (float)centerY), r,
            IM_COL32(200, 100, 255, 255), 0, 1.5f);
    }

    if (settings.espEnabled) {
        const auto& pawns = cachedPawns;
        for (size_t p = 0; p < pawns.size(); p++) {
            uintptr_t pawn = pawns[p];
            if (pawn == localPawn) 
                continue;

            oldOrigin player = read<oldOrigin>(pawn + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin);
            int health = read<int>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);
            int team = 0;
            auto mIt = metaCache.find(pawn);
            if (mIt != metaCache.end()) team = mIt->second.team;
            if (settings.teamCheck && team == lTeam) 
                continue;
            if (!health || health > 100) 
                continue;

            Vector3 feetPos = { player.cords[0], player.cords[1], player.cords[2] };
            Vector3 headPos = { feetPos.x, feetPos.y, feetPos.z + 65.0f };
            Vector2 feet, head;

            float dx = localEye.x - feetPos.x, dy = localEye.y - feetPos.y, dz = localEye.z - feetPos.z;
            float distF = sqrtf(dx * dx + dy * dy + dz * dz);
            if (std::isnan(distF)) continue;
            int distance = (int)(distF * 0.01905f);

            if (!worldToScreen(feetPos, feet, viewMatrix.data(), W, H))
                continue;
            if (!worldToScreen(headPos, head, viewMatrix.data(), W, H)) 
                continue;

            int topY = (int)head.y, bottomY = (int)feet.y;
            if (topY > bottomY) std::swap(topY, bottomY);
            topY -= 5;
            int boxH = bottomY - topY;
            if (boxH < 5) continue;
            int boxW = (int)(boxH / 2.25f);
            int leftX = (int)(head.x - boxW / 2);
            int rightX = (int)(head.x + boxW / 2);

            if (settings.showBox) {
                draw_list->AddRect(ImVec2((float)leftX, (float)topY),
                    ImVec2((float)rightX, (float)bottomY),
                    IM_COL32(200, 100, 255, 255), 0.0f, 0, 1.5f);
            }

            if (settings.showHealth) {
                int hbX = leftX - 6;
                for (int i = 0; i < boxH; i++) {
                    float ph = 100.0f - ((float)i / boxH) * 100.0f;
                    if (ph > health) 
                        continue;
                    int r, g, b;
                    if (ph > 50) { r = (int)(255 * (100 - ph) / 50.0f); g = 255; }
                    else { r = 255; g = (int)(255 * ph / 50.0f); }
                    b = 0;
                    draw_list->AddLine(ImVec2((float)hbX, (float)(topY + i)),
                        ImVec2((float)(hbX + 3), (float)(topY + i)),
                        IM_COL32(r, g, b, 255), 1.0f);
                }
            }

            if (settings.showNames) {
                const char* nameStr = "";
                if (mIt != metaCache.end() && mIt->second.nameValid) nameStr = mIt->second.name;
                draw_list->AddText(ImVec2((float)head.x - ImGui::CalcTextSize(nameStr).x / 2,
                    (float)topY - 18),
                    IM_COL32(200, 100, 255, 255), nameStr);
            }

            if (settings.showDistance) {
                char distStr[32]; sprintf_s(distStr, "%dm", distance);
                draw_list->AddText(ImVec2((float)head.x - ImGui::CalcTextSize(distStr).x / 2,
                    (float)bottomY + 5),
                    IM_COL32(200, 100, 255, 255), distStr);
            }

            if (!settings.showBones) 
                continue;

            uintptr_t boneArray = 0;
            auto bit = boneArrayCache.find(pawn);
            if (bit != boneArrayCache.end()) boneArray = bit->second;
            else {
                uintptr_t gsn = read<uintptr_t>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
                if (gsn > 0x10000 && gsn < 0x00007FFFFFFFFFFF) {
                    boneArray = read<uintptr_t>(gsn + 0x1c0);
                    if (boneArray > 0x10000 && boneArray < 0x00007FFFFFFFFFFF)
                        boneArrayCache[pawn] = boneArray;
                }
            }
            if (!boneArray) continue;

            static const int bonePairs[][2] = {
                { 7,6 },{ 6,4 },{ 4,3 },{ 3,2 },{ 2,1 },
                { 4,12 },{ 12,13 },{ 13,14 },{ 14,15 },{ 4,8 },
                { 8,9 },{ 9,10 },{ 10,11 },{ 1,20 },{ 20,21 },
                { 21,22 },{ 1,17 },{ 17,18 },{ 18,19 },
            };
            Vector2 sb[23] = {}; bool valid[23] = { false };
            if (readBoneArray(boneArray, 22)) {
                for (int i = 1; i <= 22; i++) {
                    Vector3 bp;
                    if (!getBoneFromBuffer(i, bp)) 
                        continue;
                    if (!worldToScreen(bp, sb[i], viewMatrix.data(), W, H)) continue;
                    if (sb[i].x >= 0 && sb[i].x <= W && sb[i].y >= 0 && sb[i].y <= H) valid[i] = true;
                }
            }
            for (auto& pr : bonePairs)
                if (valid[pr[0]] && valid[pr[1]])
                    draw_list->AddLine(ImVec2(sb[pr[0]].x, sb[pr[0]].y),
                        ImVec2(sb[pr[1]].x, sb[pr[1]].y),
                        IM_COL32(255, 0, 0, 255), 1.5f);
            if (valid[7])
                draw_list->AddCircle(ImVec2(sb[7].x, sb[7].y), 5.0f,
                    IM_COL32(255, 0, 0, 255), 0, 1.5f);
        }
    }

    runAimbot(clientBase, localPawn, cachedPawns, boneArrayCache, metaCache, localEye, lTeam);
    RunTriggerbot(clientBase, localPawn, entityList, lTeam, localHealth);
}


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

inline LRESULT CALLBACK OverlayWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(h, m, w, l)) 
        return true;
    return DefWindowProc(h, m, w, l);
}

inline bool CreateOverlayDevice(HWND hwnd, int w, int h) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL fl;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &swapChain, &pd3dDevice, &fl, &pd3dDeviceContext);
    if (FAILED(hr)) 
        return false;

    ID3D11Texture2D* bb = nullptr;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    if (bb) {
        pd3dDevice->CreateRenderTargetView(bb, nullptr, &mainRenderTargetView);
        bb->Release();
    }
    return mainRenderTargetView != nullptr;
}

inline HWND SetupOverlay() {
    HWND game = nullptr;
    while (!game) {
        EnumWindows([](HWND w, LPARAM lp) -> BOOL {
            DWORD pid = 0; GetWindowThreadProcessId(w, &pid);
            if (pid != GetCurrentProcessId()) return TRUE;
            if (!IsWindowVisible(w)) return TRUE;
            RECT r; GetClientRect(w, &r);
            if (r.right <= 100 || r.bottom <= 100) return TRUE;
            char cls[256]; GetClassNameA(w, cls, 256);
            if (strcmp(cls, "ConsoleWindowClass") == 0) return TRUE;
            *(HWND*)lp = w;
            return FALSE;
            }, (LPARAM)&game);
        if (!game) Sleep(500);
    }

    RECT cr; GetClientRect(game, &cr);
    POINT tl = { 0, 0 }; ClientToScreen(game, &tl);

    WNDCLASSEX wc = { sizeof(wc) };
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"DiscordOverlay";
    RegisterClassEx(&wc);

    hwndOverlay = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        wc.lpszClassName, L"", WS_POPUP,
        tl.x, tl.y, cr.right, cr.bottom,
        nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwndOverlay) return nullptr;

    SetLayeredWindowAttributes(hwndOverlay, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwndOverlay, SW_SHOW);

    if (!CreateOverlayDevice(hwndOverlay, cr.right, cr.bottom)) 
        return nullptr;

    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwndOverlay);
    ImGui_ImplDX11_Init(pd3dDevice, pd3dDeviceContext);

    return hwndOverlay;
}

inline void OverlayFrame() {
    if (!pd3dDevice || !pd3dDeviceContext || !swapChain || !mainRenderTargetView)
        return;
    if (!ImGui::GetCurrentContext())
        return;

    handleInputs();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DrawMenu();
    DrawESP(swapChain);
    DrawRadar();
    ImGui::Render();

    pd3dDeviceContext->OMSetRenderTargets(1, &mainRenderTargetView, nullptr);
    const float clear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    pd3dDeviceContext->ClearRenderTargetView(mainRenderTargetView, clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    swapChain->Present(0, 0);
}

inline void TeardownOverlay() {
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
    if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = nullptr; }
    if (swapChain) { swapChain->Release();            swapChain = nullptr; }
    if (pd3dDeviceContext) { pd3dDeviceContext->Release();    pd3dDeviceContext = nullptr; }
    if (pd3dDevice) { pd3dDevice->Release();           pd3dDevice = nullptr; }
    if (hwndOverlay && IsWindow(hwndOverlay)) DestroyWindow(hwndOverlay);
    hwndOverlay = nullptr;
}

#include "Radar.h"