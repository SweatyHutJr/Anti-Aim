#pragma once
#include <cmath>
#include <vector>
#include <unordered_map>

#include "WorldToScreen.h"
#include "BoneBoxESP.h"
#include "offsets.hpp"
#include "buttons.hpp"     =

inline QAngle calcAngle(const Vector3& src, const Vector3& dst) {
    QAngle angles;
    Vector3 delta = { dst.x - src.x, dst.y - src.y, dst.z - src.z };
    float hyp = sqrtf(delta.x * delta.x + delta.y * delta.y);
    angles.x = atan2f(-delta.z, hyp) * (180.0f / 3.14159265358979323846f);
    angles.y = atan2f(delta.y, delta.x) * (180.0f / 3.14159265358979323846f);
    angles.z = 0.0f;
    return angles;
}

inline void runAimbot(uintptr_t clientBase,
    uintptr_t local_pawn,
    const std::vector<uintptr_t>& cachedPawns,
    std::unordered_map<uintptr_t, uintptr_t>& boneArrayCache,
    const std::unordered_map<uintptr_t, PlayerMeta>& metaCache,
    const Vector3& localEye,
    int localteam)
{
    if (!settings.aimbotEnabled) 
        return;
    if (!(GetAsyncKeyState(VK_RBUTTON) & 0x8000))
        return;

    float bestFov = settings.aimbotFov;
    Vector3 bestTargetPos = { 0, 0, 0 };
    bool targetFound = false;
    QAngle currentAngle = read<QAngle>(clientBase + cs2_dumper::offsets::client_dll::dwViewAngles);

    for (uintptr_t CSPlayerPawnPtr : cachedPawns) {
        if (CSPlayerPawnPtr == local_pawn) continue;

        int team = localteam;
        auto mit = metaCache.find(CSPlayerPawnPtr);
        if (mit != metaCache.end()) team = mit->second.team;

        if (!settings.aimbotTargetTeammates && team == localteam)
            continue;

        int health = read<int>(CSPlayerPawnPtr + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);
        if (health <= 0 || health > 100)
            continue;

        uintptr_t boneArray = 0;
        auto bit = boneArrayCache.find(CSPlayerPawnPtr);
        if (bit != boneArrayCache.end()) boneArray = bit->second;
        if (boneArray == 0) {
            uintptr_t gsn = read<uintptr_t>(CSPlayerPawnPtr + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
            if (gsn > 0x10000 && gsn < 0x00007FFFFFFFFFFF) {
                boneArray = read<uintptr_t>(gsn + 0x1c0);
                if (boneArray > 0x10000 && boneArray < 0x00007FFFFFFFFFFF)
                    boneArrayCache[CSPlayerPawnPtr] = boneArray;
            }
        }
        if (boneArray == 0)
            continue;

        Vector3 headPos;
        if (!getBonePosition(CSPlayerPawnPtr, 7, headPos, boneArray))
            continue;

        if (settings.aimbotVisibleOnly && team != localteam) {
            bool isSpotted = read<bool>(CSPlayerPawnPtr + cs2_dumper::schemas::client_dll::C_CSPlayerPawn::m_entitySpottedState + 0x8);
            if (!isSpotted)
                continue;
        }

        QAngle targetAngle = calcAngle(localEye, headPos);
        float fovX = std::abs(targetAngle.x - currentAngle.x);

        // Normalize yaw delta to [-180, 180] to handle wrap-around
        float dy = targetAngle.y - currentAngle.y;
        while (dy > 180.0f) dy -= 360.0f;
        while (dy < -180.0f) dy += 360.0f;
        float fovY = std::abs(dy);

        float totalFov = sqrtf(fovX * fovX + fovY * fovY);

        if (totalFov < bestFov) {
            bestFov = totalFov;
            bestTargetPos = headPos;
            targetFound = true;
        }
    }

    if (targetFound) {
        QAngle targetAngle = calcAngle(localEye, bestTargetPos);
        while (targetAngle.y > 180.0f) targetAngle.y -= 360.0f;
        while (targetAngle.y < -180.0f) targetAngle.y += 360.0f;

        QAngle newAngle;
        float smooth = (settings.aimbotSmooth > 0.0f) ? settings.aimbotSmooth : 1.0f;
        newAngle.x = currentAngle.x + (targetAngle.x - currentAngle.x) / smooth;
        newAngle.y = currentAngle.y + (targetAngle.y - currentAngle.y) / smooth;
        newAngle.z = 0.0f;

        write(clientBase + cs2_dumper::offsets::client_dll::dwViewAngles, newAngle);
    }
}