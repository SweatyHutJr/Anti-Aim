#pragma once
#include <cstdint>
#include <Windows.h>
#include "bhop.h"
#include "buttons.hpp"

inline void RunTriggerbot(uintptr_t clientBase, uintptr_t local_pawn, uintptr_t entityList, int localTeam, int localHealth) {
    // Reset state when triggerbot is disabled
    if (!settings.triggerbotEnabled) {
        if (settings.isTriggering) {
            write(clientBase + cs2_dumper::buttons::attack, 256);
            settings.isTriggering = false;
        }
        return;
    }

    bool shouldShoot = false;
    if (localHealth > 0) {
        int crosshairEntIndex = read<int>(local_pawn + cs2_dumper::schemas::client_dll::C_CSPlayerPawn::m_iIDEntIndex);
        if (crosshairEntIndex != -1) {
            uintptr_t triggerListEntry2 = read<uintptr_t>(entityList + 0x8 * ((crosshairEntIndex & 0x7FFF) >> 9) + 0x10);
            if (triggerListEntry2) {
                uintptr_t targetPawn = read<uintptr_t>(triggerListEntry2 + 0x70 * (crosshairEntIndex & 0x1FF));
                if (targetPawn) {
                    int targetHealth = read<int>(targetPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);
                    int targetTeam = read<int>(targetPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
                    if (targetHealth > 0 && targetHealth <= 100) {
                        if (!settings.teamCheck || targetTeam != localTeam)
                            shouldShoot = true;
                    }
                }
            }
        }
    }


    static DWORD crosshairEnterTime = 0;

    if (shouldShoot) {
        if (crosshairEnterTime == 0) {
            crosshairEnterTime = GetTickCount();
        }

        DWORD elapsed = GetTickCount() - crosshairEnterTime;
        if (elapsed < (DWORD)settings.triggerDelay)
            return;   // not yet — wait

        if (!settings.isTriggering) {
            write(clientBase + cs2_dumper::buttons::attack, 65537);
            settings.isTriggering = true;
        }
    }
    else {
        crosshairEnterTime = 0; 

        if (settings.isTriggering) {
            write(clientBase + cs2_dumper::buttons::attack, 256);
            settings.isTriggering = false;
        }
    }
}