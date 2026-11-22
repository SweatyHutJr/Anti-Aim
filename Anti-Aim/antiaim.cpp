#include "antiaim.h"
#include <cmath>

namespace F::RAGEBOT::ANTIAIM
{
    static float s_spinAngle = 0.f;
    static bool  s_flip = false;

    bool CanRun(CBaseUserCmdPB* pBaseCmd, CCSPlayerController*, C_CSPlayerPawn*)
    {
        if (!settings.enabled) 
            return false;
        if (!pBaseCmd)
            return false;
        return true;
    }

    // Re-derive forward/side from world-space velocity so movement never breaks
    static void MovementFix(CBaseUserCmdPB* pBaseCmd,
        const QAngle_t& real, const QAngle_t& fake)
    {
        if (!settings.moveFix)
            return;

        float fm = pBaseCmd->forwardmove;
        float sm = pBaseCmd->leftmove;
        if (std::fabsf(fm) < 0.01f && std::fabsf(sm) < 0.01f)
            return;

        constexpr float DEG2RAD = 3.14159265f / 180.f;
        float realRad = real.y * DEG2RAD;
        float fakeRad = fake.y * DEG2RAD;

        // 1. Real forward/side -> world direction
        float wX = fm * cosf(realRad) - sm * sinf(realRad);
        float wY = fm * sinf(realRad) + sm * cosf(realRad);

        // 2. World direction -> fake forward/side
        float newFm = wX * cosf(fakeRad) + wY * sinf(fakeRad);
        float newSm = -wX * sinf(fakeRad) + wY * cosf(fakeRad);

        pBaseCmd->forwardmove = newFm;
        pBaseCmd->leftmove = newSm;

        pBaseCmd->has_bits |= (CBASEUSERCMDPB_BITS_FORWARDMOVE
            | CBASEUSERCMDPB_BITS_LEFTMOVE);
    }

    void OnMove(CBaseUserCmdPB* pBaseCmd, CCSPlayerController*, C_CSPlayerPawn*)
    {
        if (!CanRun(pBaseCmd, nullptr, nullptr))
            return;

        CMsgQAngle* va = pBaseCmd->viewangles;
        if (!va)
            return;

        QAngle_t real(va->x, va->y, va->z);
        actual_angles = real;
        g_lastCmd = pBaseCmd;
        g_lastVa = va;

        float yawOffset = 0.f;

        switch (settings.mode)
        {
        case AAMode::Jitter:
        {
            s_flip = !s_flip;
            yawOffset = s_flip ? settings.jitterAngle : -settings.jitterAngle;
            break;
        }
        case AAMode::Spin:
        {
            s_spinAngle += settings.spinSpeed;
            if (s_spinAngle > 360.f) s_spinAngle -= 360.f;
            yawOffset = s_spinAngle;
            break;
        }
        case AAMode::Static:
        {
            yawOffset = settings.yawBase;
            break;
        }
        case AAMode::TurnAround:
        {
            yawOffset = 180.f;
            break;
        }
        }

        fake_angles = QAngle_t(settings.pitch, real.y + yawOffset, 0.f);

        MovementFix(pBaseCmd, real, fake_angles);
    }
}