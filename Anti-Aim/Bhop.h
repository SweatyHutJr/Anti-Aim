// bhop.h — full corrected top section
#pragma once
#include <cstdint>
#include <cstring>
#include <Windows.h>
#include "buttons.hpp"
#include "Controls.h"
#include "client_dll.hpp"

inline bool read_raw(uintptr_t address, void* out, size_t size) {
    if (!address || !out || !size) return false;
    __try {
        memcpy(out, reinterpret_cast<const void*>(address), size);
        _ReadWriteBarrier();
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

template <typename T>
T read(uintptr_t address) {
    if (!address) return T{};
    T value{};
    if (read_raw(address, &value, sizeof(T))) return value;
    return T{};
}

template <typename T>
void write(uintptr_t address, T value) {
    if (!address) return;
    __try {
        memcpy(reinterpret_cast<void*>(address), &value, sizeof(T));
        _ReadWriteBarrier();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

inline void RunBhop(uintptr_t local_pawn, uintptr_t clientBase) {
    if (!settings.bhopEnabled) return;
    const auto flags = read<std::uint32_t>(local_pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_fFlags);
    const bool in_air = flags & (1 << 0);
    const bool space_pressed = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    const auto force_jump = read<DWORD>(clientBase + cs2_dumper::buttons::jump);

    if (space_pressed && in_air) {
        write(clientBase + cs2_dumper::buttons::jump, 65537);
    }
    else if (space_pressed && !in_air) {
        write(clientBase + cs2_dumper::buttons::jump, 256);
    }
    else if (!space_pressed && force_jump == 65537) {
        write(clientBase + cs2_dumper::buttons::jump, 256);
    }
}