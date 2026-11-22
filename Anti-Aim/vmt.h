#pragma once
#include <Windows.h>
#include <cstring>

class VmtHook
{
    void** m_vtable = nullptr;
    void* m_original = nullptr;
    int    m_slot = 0;

public:
    bool Create(void** vtable, int slot, void* hook)
    {
        if (!vtable || !hook || slot < 0) return false;

        m_vtable = vtable;
        m_slot = slot;
        m_original = vtable[slot];

        DWORD old;
        if (!VirtualProtect(&vtable[slot], sizeof(void*),
            PAGE_EXECUTE_READWRITE, &old))
            return false;

        vtable[slot] = hook;
        VirtualProtect(&vtable[slot], sizeof(void*), old, &old);
        return true;
    }

    void* GetOriginal() { return m_original; }

    void Destroy()
    {
        if (!m_vtable) return;
        DWORD old;
        if (VirtualProtect(&m_vtable[m_slot], sizeof(void*),
            PAGE_EXECUTE_READWRITE, &old))
        {
            m_vtable[m_slot] = m_original;
            VirtualProtect(&m_vtable[m_slot], sizeof(void*), old, &old);
        }
        m_vtable = nullptr;
    }
};