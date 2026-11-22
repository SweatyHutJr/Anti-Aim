#pragma once
#include "structs.h"
#include "vmt.h"

#ifndef CS_FASTCALL
#define CS_FASTCALL __fastcall
#endif

class CCSGOInput;

namespace H
{
    inline VmtHook oValidateInput;
    inline VmtHook oCreateMove;

    __int64 CS_FASTCALL ValidateInputHook(CCSGOInput* pInput);
    double  __fastcall CreateMoveHook(CCSGOInput* input, unsigned int slot,
        void* cmd);
}

namespace Hooks
{
    bool Init();
    void Shutdown();
}