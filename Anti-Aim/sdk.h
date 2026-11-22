#pragma once
#include "structs.h"   // your .txt, renamed .h

// CS2's QAngle in-game type (not the protobuf one)
struct QAngle_t
{
    float x, y, z;
    QAngle_t() : x(0), y(0), z(0) {}
    QAngle_t(float p, float ya, float r) : x(p), y(ya), z(r) {}
};

// CCSGOInput — you have to declare this yourself since your dump
// doesn't give you the class body. What you need:
class CCSGOInput
{
public:
    // vtable index 7 = ValidateInput (per asphyxia + your dump)
    // vtable index ?? = CreateMove (find by disassembly, often +0xC0 on vtable)

    QAngle_t GetViewAngles() { /* call the GetViewAngles function via resolved address */ }
    void     SetViewAngle(const QAngle_t& ang) { /* call SetViewAngles */ }
};