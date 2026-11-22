#pragma once
#include <cmath>
#include "WorldToScreen.h"
#include "Bhop.h"

static constexpr size_t BONE_STRIDE = 0x20;
static constexpr int    MAX_BONES = 128;

struct BoneBuffer {
    alignas(16) unsigned char data[MAX_BONES * BONE_STRIDE];
};

static BoneBuffer boneBuffer;

inline bool getBonePosition(std::uintptr_t pawn, int boneIndex, Vector3& out, uintptr_t boneArray) {
    if (boneIndex < 0 || boneIndex > 127)
        return false;
    Vector3 bone = read<Vector3>(boneArray + (boneIndex * BONE_STRIDE));

    if (std::isnan(bone.x) || std::isnan(bone.y) || std::isnan(bone.z))
        return false;
    if (bone.x == 0.0f && bone.y == 0.0f && bone.z == 0.0f)
        return false;
    if (std::abs(bone.x) > 15000.0f || std::abs(bone.y) > 15000.0f || std::abs(bone.z) > 15000.0f)
        return false;

    out.x = bone.x;
    out.y = bone.y;
    out.z = bone.z;
    return true;
}

inline bool readBoneArray(uintptr_t boneArray, int maxBone) {
    if (!boneArray || maxBone <= 0 || maxBone > MAX_BONES)
        return false;
    return read_raw(boneArray, boneBuffer.data, (size_t)maxBone * BONE_STRIDE);
}

inline bool getBoneFromBuffer(int boneIndex, Vector3& out) {
    if (boneIndex < 0 || boneIndex >= MAX_BONES)
        return false;

    const float* p = reinterpret_cast<const float*>(boneBuffer.data + (size_t)boneIndex * BONE_STRIDE);

    float x = p[0], y = p[1], z = p[2];
    if (std::isnan(x) || std::isnan(y) || std::isnan(z))
        return false;
    if (x == 0.0f && y == 0.0f && z == 0.0f)
        return false;
    if (std::abs(x) > 15000.0f || std::abs(y) > 15000.0f || std::abs(z) > 15000.0f)
        return false;
    out.x = x; out.y = y; out.z = z;
    return true;
} 