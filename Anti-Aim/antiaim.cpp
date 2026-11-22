#include "antiaim.h"
#include <cmath>

namespace F::RAGEBOT::ANTIAIM
{
    static bool s_flip = false;

    bool CanRun(CBaseUserCmdPB* pBaseCmd,
        CCSPlayerController* pLocalController,
        C_CSPlayerPawn* pLocalPawn)
    {
        if (!settings.enabled) return false;
        if (!pBaseCmd) return false;
        return true;
    }

    static void MovementFix(CBaseUserCmdPB* pBaseCmd,
        const QAngle_t& real, const QAngle_t& fake)
    {
        if (!settings.moveFix) return;
        float delta = real.y - fake.y;
        if (std::fabsf(delta) < 0.01f) return;

        float rad = delta * (3.14159265f / 180.f);
        float c = cosf(rad);
        float s = sinf(rad);
        float fm = pBaseCmd->forwardmove;
        float sm = pBaseCmd->leftmove;

        pBaseCmd->forwardmove = fm * c - sm * s;
        pBaseCmd->leftmove = fm * s + sm * c;

        pBaseCmd->has_bits |= (CBASEUSERCMDPB_BITS_FORWARDMOVE
            | CBASEUSERCMDPB_BITS_LEFTMOVE);
    }

    void OnMove(CBaseUserCmdPB* pBaseCmd,
        CCSPlayerController* pLocalController,
        C_CSPlayerPawn* pLocalPawn)
    {
        if (!CanRun(pBaseCmd, pLocalController, pLocalPawn))
            return;

        CMsgQAngle* va = pBaseCmd->viewangles;
        if (!va) return;

        QAngle_t real(va->x, va->y, va->z);
        actual_angles = real;
        g_lastCmd = pBaseCmd;
        g_lastVa = va;

        s_flip = !s_flip;
        float yawOffset = s_flip ? settings.yawBase : -settings.yawBase;
        if (!settings.jitter)
            yawOffset = settings.yawBase;

        fake_angles = QAngle_t(settings.pitch, real.y + yawOffset, 0.f);

        MovementFix(pBaseCmd, real, fake_angles);
    }
}