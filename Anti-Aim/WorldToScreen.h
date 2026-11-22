#pragma once

struct Vector3 { float x, y, z; };
struct Vector2 { float x, y; };
struct QAngle { float x, y, z; };
struct oldOrigin { float cords[3]; };
struct nameBuf { char data[128]; };

// OPT: per-player cached metadata. Refreshed only when the pawn list is rebuilt
// (every ~3s), not every frame. Team never changes mid-round; name rarely does.
struct PlayerMeta {
    int  team = 0;
    char name[128] = {};
    bool nameValid = false;
};

inline bool worldToScreen(const Vector3& pos, Vector2& screen, const float matrix[16], int windowWidth, int windowHeight) {
    float w = matrix[12] * pos.x + matrix[13] * pos.y + matrix[14] * pos.z + matrix[15];
    if (w < 0.01f)
        return false;

    float x = matrix[0] * pos.x + matrix[1] * pos.y + matrix[2] * pos.z + matrix[3];
    float y = matrix[4] * pos.x + matrix[5] * pos.y + matrix[6] * pos.z + matrix[7];

    screen.x = (windowWidth / 2.0f) * (x / w + 1.0f);
    screen.y = (windowHeight / 2.0f) * (1.0f - (y / w));
    return true;
}