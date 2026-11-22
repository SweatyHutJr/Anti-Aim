#pragma once

#include <Windows.h>
#include <cstdio>
#include <string>

struct cheatSettings {
    bool espEnabled = true;
    bool teamCheck = true;
    bool triggerbotEnabled = false;
    bool aimbotEnabled = false;
    bool aimbotVisibleOnly = false;
    bool aimbotTargetTeammates = false;
    bool bhopEnabled = true;
    float aimbotSmooth = 1.0f;
    float aimbotFov = 10.0f;
    bool isTriggering = false;
    float triggerDelay = 0.0f;
    bool rcsEnabled = false;
    float rcsScale = 1.0f;

    bool showBox = true;
    bool showNames = true;
    bool showDistance = false;
    bool showBones = false;
    bool showHealth = true;
    bool showFovCircle = true;

    bool  radarEnabled = false;
    bool  radarTeamCheck = true;
    float radarRange = 3000.0f;
    float radarSize = 150.0f;
    float radarOpacity = 0.70f;
    float radarOffsetX = 0.0f;
    float radarOffsetY = 0.0f;

    // Anti-aim
    bool  antiaimEnabled = true;
    int   antiaimMode = 1;     // 0=Jitter, 1=Spin, 2=Static, 3=TurnAround
    float antiaimPitch = 89.f;
    float antiaimYaw = 179.f;
    float antiaimSpinSpeed = 6.f;
    float antiaimJitterAngle = 20.f;
    bool  antiaimMoveFix = true;
} inline settings;

inline void handleInputs() {
    if (GetAsyncKeyState(VK_F1) & 1) settings.espEnabled = !settings.espEnabled;
    if (GetAsyncKeyState(VK_F2) & 1) settings.teamCheck = !settings.teamCheck;
    if (GetAsyncKeyState(VK_F3) & 1) settings.triggerbotEnabled = !settings.triggerbotEnabled;
    if (GetAsyncKeyState(VK_F4) & 1) settings.aimbotEnabled = !settings.aimbotEnabled;
    if (GetAsyncKeyState(VK_F5) & 1) settings.bhopEnabled = !settings.bhopEnabled;

    if (GetAsyncKeyState(VK_OEM_MINUS) & 1) {
        settings.aimbotSmooth -= 1.0f;
        if (settings.aimbotSmooth < 1.0f) settings.aimbotSmooth = 1.0f;
    }
    if (GetAsyncKeyState(VK_OEM_PLUS) & 1) {
        settings.aimbotSmooth += 1.0f;
        if (settings.aimbotSmooth > 15.0f) settings.aimbotSmooth = 15.0f;
    }
    if (GetAsyncKeyState(VK_OEM_4) & 1) {
        settings.aimbotFov -= 1.0f;
        if (settings.aimbotFov < 1.0f) settings.aimbotFov = 1.0f;
    }
    if (GetAsyncKeyState(VK_OEM_6) & 1) {
        settings.aimbotFov += 1.0f;
        if (settings.aimbotFov > 45.0f) settings.aimbotFov = 45.0f;
    }
}