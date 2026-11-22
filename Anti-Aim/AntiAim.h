#pragma once
#include "structs.h"
#include <cstdint>

struct QAngle_t
{
    float x, y, z;
    QAngle_t() : x(0), y(0), z(0) {}
    QAngle_t(float p, float ya, float r) : x(p), y(ya), z(r) {}
};

class CCSPlayerController;
class C_CSPlayerPawn;

namespace F::RAGEBOT::ANTIAIM
{
    inline QAngle_t actual_angles{};
    inline QAngle_t fake_angles{};

    // Saved by OnMove, used by ValidateInputHook
    inline CBaseUserCmdPB* g_lastCmd = nullptr;
    inline CMsgQAngle* g_lastVa = nullptr;

    struct Settings_t
    {
        bool  enabled = true;
        bool  jitter = true;
        bool  moveFix = true;
        float pitch = 89.f;
        float yawBase = 179.f;
    };
    inline Settings_t settings{};

    bool CanRun(CBaseUserCmdPB* pBaseCmd,
        CCSPlayerController* pLocalController,
        C_CSPlayerPawn* pLocalPawn);

    void OnMove(CBaseUserCmdPB* pBaseCmd,
        CCSPlayerController* pLocalController,
        C_CSPlayerPawn* pLocalPawn);
}