#pragma once
#include "structs.h"  


struct QAngle_t
{
    float x, y, z;
    QAngle_t() : x(0), y(0), z(0) {}
    QAngle_t(float p, float ya, float r) : x(p), y(ya), z(r) {}
};


class CCSGOInput
{
public:


    QAngle_t GetViewAngles() { /* call the GetViewAngles function via resolved address */ }
    void     SetViewAngle(const QAngle_t& ang) { /* call SetViewAngles */ }
};