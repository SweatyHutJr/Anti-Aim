#pragma once
#include "menu.h"

inline void DrawRadar() {
    if (!settings.radarEnabled) return;
    if (!clientBase) return;

    uintptr_t localPawn = read<uintptr_t>(clientBase + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;

    oldOrigin localPos = read<oldOrigin>(localPawn + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin);
    int localTeam = read<int>(localPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);

    QAngle va = read<QAngle>(clientBase + cs2_dumper::offsets::client_dll::dwViewAngles);
    const float yawRad = va.y * (3.14159265f / 180.0f);
    const float cy = cosf(yawRad);
    const float sy = sinf(yawRad);

    const float R = settings.radarSize;
    const float M = 20.0f;
    const float CX = settings.radarOffsetX + R + M;
    const float CY = settings.radarOffsetY + R + M;

    const float range = settings.radarRange;
    const float k = R / range;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    const int bgAlpha = (int)(255.0f * settings.radarOpacity);
    dl->AddCircleFilled(ImVec2(CX, CY), R, IM_COL32(20, 20, 20, bgAlpha));
    dl->AddCircle(ImVec2(CX, CY), R, IM_COL32(200, 100, 255, 255), 0, 1.5f);

    dl->PathArcTo(ImVec2(CX, CY), R, -1.5707963f - 0.5236f, -1.5707963f + 0.5236f, 24);
    dl->PathStroke(IM_COL32(100, 255, 100, 60), false, 2.0f);


    dl->AddCircleFilled(ImVec2(CX, CY), 5.0f, IM_COL32(100, 255, 100, 255));
    dl->AddCircle(ImVec2(CX, CY), 5.0f, IM_COL32(255, 255, 255, 220), 1.0f);

    for (uintptr_t pawn : cachedPawns) {
        if (pawn == localPawn) continue;

        auto mIt = metaCache.find(pawn);
        int team = (mIt != metaCache.end()) ? mIt->second.team : 0;
        if (settings.radarTeamCheck && team == localTeam) continue;

        int health = read<int>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);
        if (health <= 0 || health > 100) continue;

        oldOrigin ep = read<oldOrigin>(pawn + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin);

        float dx = ep.cords[0] - localPos.cords[0];
        float dy = ep.cords[1] - localPos.cords[1];
        if (dx * dx + dy * dy > range * range) 
            continue;

        float fwd = dx * cy + dy * sy;
        float right = dx * sy - dy * cy;

        float screenX = CX + right * k;
        float screenY = CY - fwd * k;

        ImU32 color = (team == localTeam) ? IM_COL32(255, 200, 100, 255)
            : IM_COL32(255, 60, 60, 255);
        float dotSize = 1.5f + (health / 100.0f) * 2.0f;
        dl->AddCircleFilled(ImVec2(screenX, screenY), dotSize, color);
    }
}