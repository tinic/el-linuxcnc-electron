# Elle

Electronic leadscrew and canned cycles for CNC-converted lathes on LinuxCNC. See README.md for the
architecture and build commands.

## Layout

- `backend/src/core`: pure C++ (no I/O): cycle planning and G-code, validation, leadscrew table,
  jog dead-man, `LatheController` (work/tool offsets, encoder resets), backplot, `TaskPort`.
- `backend/src/linuxcnc`: HAL component `lathe` and the NML `TaskClient`. Only these files include
  LinuxCNC headers.
- `backend/src/app`: JSON protocol (glaze), `Session`, `TaskWorker`, `ControlLoop`, settings, INI.
- `backend/src/shell`: `elle` executable and the WebKitGTK window. Only `panel_window.cpp`
  includes GTK/WebKit.
- `ui/`: Vue 3 + TypeScript panel. `src/protocol.ts` mirrors `backend/src/app/.../protocol.hpp`.
- `machine/`: LinuxCNC config. Nets on `lathe.*` pins belong in `postgui.hal`.

## Rules

- Strict C++23, GCC 16. Every GCC warning in `cmake/ProjectOptions.cmake` is an error, and
  clang-tidy runs with all checks. Fix the code; never relax a check globally. An exclusion needs
  a reason in `.clang-tidy`, and a local suppression needs a reason comment.
- No Python, Electron or PrimeVue.
- Units: millimetres everywhere, X as a radius. Conversion to inches or diameter happens only for
  display, in `ui/src/units.ts`.
- G-code output is pinned by `backend/test/golden/cycles.json`. A change that alters it must
  update the golden data deliberately and record the reason in `backend/test/golden/README.md`.
- Protocol changes touch both `protocol.hpp`/`protocol.cpp` and `ui/src/protocol.ts`, and the
  simulator in `ui/src/simulator.ts`.
- Safety: jog needs the panel's 50 ms refresh (dead-man 150 ms); cycles start only from a fresh
  preview in canned-cycle mode. Keep both.

## Checks

```sh
cd backend && for p in dev tsan release; do cmake --workflow --preset check-$p; done
cmake --workflow --preset check-analyze     # needs clang-tidy-19, cppcheck, iwyu
cd ../ui && npm run check
```

Use `elle --demo --windowed` to try the real stack without a machine. Do not launch LinuxCNC with
HAL on the lathe without the owner's go-ahead.
