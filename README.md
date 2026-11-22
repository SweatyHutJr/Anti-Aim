
## Requirements

- **Visual Studio 2022** (v143 toolset), C++20
- **Windows 10/11** x64
- CS2 installed (Steam)
- DirectX 11 SDK
- ImGui (included under `ImGUI/`)

## Building

1. Clone the repository.
2. Open `Cheat.sln` in Visual Studio 2022.
3. Set configuration to **Release | x64**.
4. Build → Rebuild Solution.
5. Output: `Release/Cheat.dll`.

The project uses `d3d11.lib`, `dxgi.lib`, and `d3dcompiler.lib` via
`#pragma comment` — no extra linker config needed.

## Injecting

- **For testing**: use a **standard `LoadLibrary` injector** (e.g., Process
  Hacker 2, Xenos, or your own). Do **not** use manual map for first-time
  testing — global constructors don't run under manual map and the
  `inline` VmtHook objects won't be initialized.
- The cheat waits for `client.dll` to load, sleeps 2 seconds to let the input
  system initialize, then installs hooks.
- A console window opens with diagnostic output. Press **F11** to unload.

## Usage

### Menu

Press **INSERT** to toggle the menu. The overlay automatically becomes
click-through when the menu is hidden.

### Keybinds (default)

| Key | Action |
|---|---|
| INSERT | Toggle menu |
| F1 | Toggle ESP |
| F2 | Toggle team check |
| F3 | Toggle triggerbot |
| F4 | Toggle aimbot |
| F5 | Toggle bunnyhop |
| F11 | Unload cheat |

## Anti-Aim

The anti-aim has four modes, selectable from the menu:

| Mode | Behavior |
|---|---|
| **Jitter** | Alternates ±`Jitter Angle` each tick (aggressive, staggers visually) |
| **Spin** | Continuous rotation by `Spin Speed` degrees per tick (smooth) |
| **Static** | Fixed yaw offset (use `Yaw Offset` slider) |
| **Turn Around** | Faces exactly 180° behind the camera |

### Recommended settings for smooth spin

| Setting | Value | Why |
|---|---|---|
| Mode | Spin | Continuous rotation, no teleport stagger |
| Pitch | 89° | Look almost straight down |
| Spin Speed | 5-6°/tick | ~320-384°/sec — one rotation per second |
| Movement Fix | On | Preserves WASD direction at all spin angles |

### How the anti-aim works

The cheat uses a two-hook system:

1. **`CreateMove` (slot 25)** — after the game populates the user command,
   `OnMove` computes the fake angles and saves `g_lastCmd` / `g_lastVa`.
2. **`ValidateInput` (slot 8)** — the hook calls the original (so the camera
   stays on your real mouse look), then patches `g_lastVa` with the fake
   angles. The patched command is what gets serialized to the server.

This design keeps your **camera real** (you can aim normally) while the
**server sees spinning angles** (defeats enemy aimbot / resolver).

### Movement Fix

The original naive movement fix (`rotate forward/side by delta`) breaks
movement when the yaw delta crosses ±90°. The current implementation
re-derives the intended **world-space velocity** from the real angle, then
re-projects it onto the fake angle's forward/side axes. This keeps WASD
working at every anti-aim angle.

## Configuration

All settings are edited live in the ImGui menu. There is no config file
system yet — settings reset on re-inject.

## Regenerating Offsets

After each CS2 update, `offsets.hpp` and `client_dll.hpp` become stale.
Regenerate them:

1. Go to [github.com/a2x/cs2-dumper](https://github.com/a2x/cs2-dumper).
2. Download the latest `offsets.hpp` and `client_dll.hpp`.
3. Replace the files in the project root.
4. Rebuild.

The `CreateMove` signature and `ValidateInput` slot index typically survive
minor updates, but verify after major patches:

- `CreateMove` pattern: `48 8B C4 4C 89 40 ? 48 89 48 ? 55 53 57 41 55`
- `ValidateInput` slot: `8` (confirmed on current build)

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Cheat doesn't inject | DLL not built, or injector failed | Rebuild, verify DLL path, use standard `LoadLibrary` injector |
| Console shows `input is null` | `dwCSGOInput` offset stale | Re-dump offsets from cs2-dumper |
| `Hooks OK` printed but no ESP | `dwEntityList` / `dwViewMatrix` stale | Re-dump offsets |
| `CM=0` in console | `CreateMove` hook not installed | Verify `vtable[25]` is `CreateMove` — check bytes |
| `VI=0` in console | `ValidateInput` hook not installed | Try slot 8, then 7, 9 |
| Anti-aim camera spins | Patching before `original()` | Ensure `ValidateInputHook` calls `original` first |
| Movement freezes at some angles | Naive `MovementFix` | Use the world-vector re-projection version |
| Game freezes at Phase 2 | Called `vtable[7]` / `vtable[9]` as Get/SetViewAngles | Don't call those — patch `g_lastVa` directly |
| Game crashes on inject | VMT patcher passed wrong pointer | Verify `VmtHook::Create(vtable, slot, hook)` |

## Credits

- **asphyxia-cs2** — hooking architecture and VMT pattern
- **cs2-dumper** (a2x) — offset generation
- **ImGui** — overlay UI
- **UnknownCheats** community — anti-aim research and discussions

## License

This project is provided **as-is** for educational purposes. No warranty is
provided. You are solely responsible for how you use this code. Redistribution
of compiled binaries without source is discouraged.

---

**Last updated**: current build target — CS2 (build 14073, October 2025)

*If you are reading this and considering using it online: don't. It's not
worth the ban.*
