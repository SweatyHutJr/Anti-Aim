#include "hooks.h"
#include "antiaim.h"
#include "offsets.hpp"
#include <Windows.h>
#include <atomic>
#include <cstdio>
#include "menu.h"

#define AA_DEBUG_PHASE 2

#define CREATEMOVE_PATTERN "48 8B C4 4C 89 40 ? 48 89 48 ? 55 53 57 41 55"

using CreateMoveFn = double(__fastcall*)(CCSGOInput*, unsigned int, void*);
using ValidateInputFn = __int64(__fastcall*)(CCSGOInput*);

std::atomic<int> g_createMoveCalls{ 0 };
std::atomic<int> g_validateInputCalls{ 0 };
std::atomic<int> g_onMoveRuns{ 0 };

static uintptr_t ScanSig(const char* module, const char* sig)
{
    HMODULE h = GetModuleHandleA(module);
    if (!h) return 0;
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(h);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(
        reinterpret_cast<uintptr_t>(h) + dos->e_lfanew);
    auto* sec = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec)
    {
        if (!(sec->Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        uint8_t* base = reinterpret_cast<uint8_t*>(h) + sec->VirtualAddress;
        uint8_t* end = base + sec->Misc.VirtualSize;
        const char* p = sig;
        uint8_t bytes[256]; bool mask[256]; int n = 0;
        while (*p && n < 256) {
            while (*p == ' ') ++p;
            if (!*p) break;
            if (*p == '?') { bytes[n] = 0; mask[n] = false; ++n; ++p; if (*p == '?') ++p; continue; }
            auto hex = [](char c) {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return 0;
                };
            bytes[n] = (hex(p[0]) << 4) | hex(p[1]);
            mask[n] = true; ++n; p += 2;
        }
        for (uint8_t* cur = base; cur + n <= end; ++cur) {
            bool hit = true;
            for (int i = 0; i < n; ++i) if (mask[i] && cur[i] != bytes[i]) { hit = false; break; }
            if (hit) return reinterpret_cast<uintptr_t>(cur);
        }
    }
    return 0;
}

static bool IsReadable(const void* ptr, size_t len)
{
    if (!ptr) return false;
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(ptr, &mbi, sizeof(mbi))) return false;
    if (mbi.State != MEM_COMMIT) return false;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
    uintptr_t start = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t end = start + len;
    uintptr_t regStart = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
    uintptr_t regEnd = regStart + mbi.RegionSize;
    return end <= regEnd;
}

double __fastcall H::CreateMoveHook(CCSGOInput* input, unsigned int slot, void* cmd)
{
    g_createMoveCalls++;

    // Sync ImGui settings -> AA settings
    auto& aa = F::RAGEBOT::ANTIAIM::settings;
    aa.enabled = settings.antiaimEnabled;
    aa.moveFix = settings.antiaimMoveFix;
    aa.pitch = settings.antiaimPitch;
    aa.yawBase = settings.antiaimYaw;
    aa.spinSpeed = settings.antiaimSpinSpeed;
    aa.jitterAngle = settings.antiaimJitterAngle;

    switch (settings.antiaimMode) {
    case 0: aa.mode = F::RAGEBOT::ANTIAIM::AAMode::Jitter;     break;
    case 1: aa.mode = F::RAGEBOT::ANTIAIM::AAMode::Spin;       break;
    case 2: aa.mode = F::RAGEBOT::ANTIAIM::AAMode::Static;     break;
    case 3: aa.mode = F::RAGEBOT::ANTIAIM::AAMode::TurnAround; break;
    }

    auto orig = reinterpret_cast<CreateMoveFn>(H::oCreateMove.GetOriginal());
    double ret = orig ? orig(input, slot, cmd) : 0.0;

#if AA_DEBUG_PHASE >= 1
    if (cmd && input)
    {
        constexpr uintptr_t BASE_OFFSET = 0x40;

        void* baseRaw = nullptr;
        if (IsReadable((void*)(reinterpret_cast<uintptr_t>(cmd) + BASE_OFFSET), 8))
            baseRaw = *reinterpret_cast<void**>(
                reinterpret_cast<uintptr_t>(cmd) + BASE_OFFSET);

        if (baseRaw)
        {
            auto* basePb = reinterpret_cast<CBaseUserCmdPB*>(baseRaw);
            if (basePb->viewangles)
            {
                g_onMoveRuns++;
                F::RAGEBOT::ANTIAIM::OnMove(basePb, nullptr, nullptr);
            }
        }
    }
#endif
    return ret;
}

__int64 CS_FASTCALL H::ValidateInputHook(CCSGOInput* pInput)
{
    g_validateInputCalls++;
    auto original = reinterpret_cast<ValidateInputFn>(H::oValidateInput.GetOriginal());
    if (!original) return 0;

#if AA_DEBUG_PHASE >= 2
    if (F::RAGEBOT::ANTIAIM::settings.enabled &&
        F::RAGEBOT::ANTIAIM::g_lastVa)
    {
        __int64 ret = original(pInput);

        CMsgQAngle* va = F::RAGEBOT::ANTIAIM::g_lastVa;
        va->x = F::RAGEBOT::ANTIAIM::fake_angles.x;
        va->y = F::RAGEBOT::ANTIAIM::fake_angles.y;
        va->z = F::RAGEBOT::ANTIAIM::fake_angles.z;
        va->has_bits |= (CMSGQANGLE_BITS_X
            | CMSGQANGLE_BITS_Y
            | CMSGQANGLE_BITS_Z);
        if (F::RAGEBOT::ANTIAIM::g_lastCmd)
            F::RAGEBOT::ANTIAIM::g_lastCmd->has_bits |= CBASEUSERCMDPB_BITS_VIEWANGLES;
        return ret;
    }
#endif

    return original(pInput);
}

static int FindSlotForAddr(void** vtable, int count, uintptr_t addr)
{
    if (!addr) return -1;
    for (int i = 0; i < count; i++)
        if (reinterpret_cast<uintptr_t>(vtable[i]) == addr)
            return i;
    return -1;
}

bool Hooks::Init()
{
    printf("[Init] BUILD MARKER v13\n");
    HMODULE hClient = GetModuleHandleA("client.dll");
    if (!hClient) return false;
    uintptr_t clientBase = reinterpret_cast<uintptr_t>(hClient);

    uintptr_t inputAddr = clientBase + cs2_dumper::offsets::client_dll::dwCSGOInput;
    void* rawInput = *reinterpret_cast<void**>(inputAddr);
    if (!rawInput) { printf("[Init] input is null\n"); return false; }
    printf("[Init] rawInput=0x%p\n", rawInput);

    void** vtable = reinterpret_cast<void**>(rawInput);

    uintptr_t cmVA = ScanSig("client.dll", CREATEMOVE_PATTERN);
    int slotCM = FindSlotForAddr(vtable, 40, cmVA);
    int SLOT_CREATEMOVE = slotCM >= 0 ? slotCM : 25;
    int SLOT_VALIDATEINPUT = 8;

    if (!H::oCreateMove.Create(vtable, SLOT_CREATEMOVE,
        reinterpret_cast<void*>(&H::CreateMoveHook)))
    {
        printf("[Init] oCreateMove.Create failed\n"); return false;
    }

    if (!H::oValidateInput.Create(vtable, SLOT_VALIDATEINPUT,
        reinterpret_cast<void*>(&H::ValidateInputHook)))
    {
        printf("[Init] oValidateInput.Create failed\n"); return false;
    }

    printf("[Init] Hooks OK (CM=%d, VI=%d)\n", SLOT_CREATEMOVE, SLOT_VALIDATEINPUT);
    return true;
}

void Hooks::Shutdown()
{
    H::oValidateInput.Destroy();
    H::oCreateMove.Destroy();
}