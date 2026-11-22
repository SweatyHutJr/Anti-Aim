#pragma once
#include "menu.h"

inline void RunRCS(uintptr_t clientBase, uintptr_t localPawn) {
    if (!settings.rcsEnabled) 
         return;
    if (settings.rcsScale <= 0.0f)
        return;
    if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
        return;


    uintptr_t punchServices = read<uintptr_t>(
        localPawn + cs2_dumper::schemas::client_dll::C_CSPlayerPawn::m_pAimPunchServices);
    if (!punchServices) return;

    QAngle aimPunch = read<QAngle>(
        punchServices + cs2_dumper::schemas::client_dll::CCSPlayer_AimPunchServices::m_unpredictableBaseAngle);


    if (aimPunch.x == 0.0f && aimPunch.y == 0.0f) {
        aimPunch = read<QAngle>(
            punchServices + cs2_dumper::schemas::client_dll::CCSPlayer_AimPunchServices::m_predictableBaseAngle);
    }

    static QAngle lastAimPunch = { 0, 0, 0 };

    if (aimPunch.x < lastAimPunch.x || aimPunch.y < lastAimPunch.y) {
        lastAimPunch = aimPunch;
        return;
    }

    float dxPunch = aimPunch.x - lastAimPunch.x;
    float dyPunch = aimPunch.y - lastAimPunch.y;


    if (fabsf(dxPunch) < 0.01f && fabsf(dyPunch) < 0.01f) {
        lastAimPunch = aimPunch;
        return;
    }

    QAngle viewAngle = read<QAngle>(clientBase + cs2_dumper::offsets::client_dll::dwViewAngles);

    QAngle newAngle;
    newAngle.x = viewAngle.x - dxPunch * settings.rcsScale;
    newAngle.y = viewAngle.y - dyPunch * settings.rcsScale;
    newAngle.z = 0.0f;

    // Clamp pitch, normalize yaw
    if (newAngle.x > 89.0f)  newAngle.x = 89.0f;
    if (newAngle.x < -89.0f) newAngle.x = -89.0f;
    while (newAngle.y > 180.0f)  newAngle.y -= 360.0f;
    while (newAngle.y < -180.0f) newAngle.y += 360.0f;

    write(clientBase + cs2_dumper::offsets::client_dll::dwViewAngles, newAngle);
    lastAimPunch = aimPunch;
}