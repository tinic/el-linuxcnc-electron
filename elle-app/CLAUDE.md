# Elle

## Description
Elle is a standalone electronic leadscrew, linuxcnc HAL configuration, Vue3 based Electron application for CNC converted lathes. It brings back manual control to lathes which have been converted to CNC. In addition in can run a few common select canned cycles which cover 99% of what is done with manual lathes.

The UI is primarily designed for touch screens, keyboard is not required. Where numbers can be entered a simple on screen number pad can be used. The interface mostly behaves like a classic DRO.

The application operates in two modes: Manual and Canned Cycle which can be switched at will. Both share a single coordinate system which can be set up in manual mode and is picked up by the canned cycles. 

## More features:

- Diameter and Radius mode are suported. 
- mm and inch can be switched on the fly. All numbers in the interface are updated dynamically.
- The threading and turning cycles include many common presets including NPT which are traditionally not easy to do on manual lathes.
- A simple jog interface is included in the manual mode.
- Axis can be set into different modes like hold (steppers are locked), idle (steppers are unpowered) and engaged (steppers are in sync with the spindle).
- The display of spindle rotation is in degrees and can be used like a rotary table.
- Before a canned cycle is run, the user is presented with an animated backplot preview of the g-code which is about to run.

## File Structure
- `elle-frontend/src/`: Contains the Vue3 application code.
    - `components/`: Vue components.
    - `composable/`: Vue composabes.
    - `assets/`: Static assets like fonts and presets for canned cycles.
    - `App.vue`: Main entry point of the application.
- `elle-electron/`: Contains the Electron application code.
    - `main.ts`: Main entry point of the Electron application. Mostly used to enable dev console.
- `elle-hal/`: Contains linuxcnc hal and ini configuration and glue python code.
    - `lathe_halcomp.py`: main rest interface used by the Vue application. Also contains code to generate canned cycles and back plots.

## Dependencies
- `vue`: Core Vue library.
- `threejs`: Used for rendering back plots.
- `primevue4`: Used for interface elements, css and icons
- `vite`: For build
- `electron`: Application packager

## Coding Style
- Follow common Typescript style guide.
- Follow the Vue style guide.
- Follow the Primevue style guide.
- Follow best python practices.
- Clean code, Duplication of code is avoided.
- Components share patterns and anticipate extensions.
- Using composables where possible.
- Split UI into components where possible.

## Commands
- `yarn start`: Start the development server and the electron app. Will also start linuxcnc + hal backend once the app is up and running.
- `yarn build`: Build the application.
- `yarn app:build`: Build the application for production.

## Hardware Requirements
- LinuxCNC real-time kernel (PREEMPT_RT or RTAI)
- Encoder feedback on spindle (quadrature A/B signals)
- Stepper drivers for X/Z axes with step/direction interface
- Touch screen (minimum 1024x768, 10" recommended)

## Development Setup
- LinuxCNC 2.8+ required for HAL component compatibility
- Python 3.8+ with Flask, waitress, linuxcnc modules
- Node.js 16+ for Vue/Electron development
- Real-time kernel for accurate timing
- Non-realtime and Real-time thread pinned to specific cores. See rt_setup.sh.

## System Architecture
- Frontend: Vue3 SPA running in Electron renderer process
- Backend: Python Flask REST API (port 8000) interfacing with LinuxCNC HAL
- HAL Integration: Custom HAL component (`lathe_halcomp.py`) bridges REST API to real-time HAL pins
- Communication: HTTP REST at 30Hz for position updates, immediate for commands
- Coordinate System: Single work coordinate system shared between manual and canned cycle modes

## HAL REST API Endpoints
- `GET /hal/hal_in`: Position and status data (30Hz polling)
- `PUT /hal/hal_out`: Control commands (pitch, enable, direction)
- `PUT /hal/threading`: Execute threading cycle
- `PUT /hal/threading/generate`: Generate G-code preview for backplot
- `PUT /hal/abort`: Abort current operation
...

## Important Behaviors
- Position updates are relative to user-set work coordinates
- Threading cycles use G33 synchronized moves requiring spindle encoder
- Manual mode allows electronic leadscrew with configurable pitch ratios
- Emergency stop preserves work coordinate system
- All measurements displayed in current unit system (mm/inch toggle)

## Settings System (single-schema)

All persisted settings are defined ONCE in `elle-electron/settings-schema.ts`
(zod). Everything else derives from it:

- **Adding a setting is a one-file change**: add a field with a `.catch(default)`
  to `SettingsSchema`. The electron-store shape, IPC payload types
  (preload.d.ts), and the frontend refs (`useSettings()` exposes `toRefs` of a
  reactive parsed from the schema) all follow automatically.
- Both IPC handlers in `main.ts` parse through the schema, so missing fields
  get defaults and corrupt values recover per-field — a damaged
  `~/.config/elle-app/appConfig.json` can never break startup.
- `saveSettings()` passes the reactive state through `SettingsSchema.parse()`,
  which yields a plain structured-clone-safe object. Never send Vue refs or
  proxies over IPC directly.
- Auto-save: a single module-level deep watcher in `useSettings.ts`. Never
  gate saving on `isQuitting` — the final save during quit must go through.
- Tests: `elle-frontend/src/composables/settings-schema.test.ts` (defaults,
  per-field corruption recovery, unknown-key stripping, round-trip).

## Complete Technical Architecture Guide

### Core Architecture & Data Flow

**Three-Layer System:**
```
Vue Frontend (renderer process) ↔ Electron Main Process ↔ Python HAL Backend ↔ LinuxCNC
```

**Communication Patterns:**
- **Position Updates**: 30Hz HTTP polling via `/hal/hal_in` endpoint
- **Control Commands**: Immediate HTTP PUT requests to `/hal/hal_out`
- **Settings**: Electron IPC (`getSettings`/`saveSettings`) → electron-store → JSON file
- **HAL Management**: IPC channels (`startHAL`, `stopHAL`, `halStarted`, `halStopped`)

### Key Components & Responsibilities

**Vue Component Hierarchy:**
```
App.vue (thin shell: menu, stack-light status, HAL lifecycle, quit)
├── ManualScreen.vue (DRO + numpad + feed/direction/jog controls)
├── CannedCyclesScreen.vue (threading + turning panels, preview dialog)
│   ├── OperationPreview.vue (3D backplot using Three.js/troisjs)
│   ├── ThreadPresetSelector.vue / TurningPresetSelector.vue (dialogs)
│   └── DRODisplay.vue / Numpad.vue (shared with ManualScreen)
├── HalConsole.vue (HAL start/stop + console output)
├── SettingsScreen.vue (display mode, units, encoder scales)
├── ToolTable.vue (tool offset editing and selection)
└── PitchPresetSelector.vue (manual pitch selection dialog)
```
Screens take NO props — all shared state comes from singleton composables.

### Critical APIs & Interfaces

**HAL REST API Endpoints (Python backend on port 8000):**
```typescript
// Position & Status (30Hz polling)
GET /hal/hal_in → {position_x, position_z, position_a, speed_rps, program_running, error_state}

// Control Commands  
PUT /hal/hal_out → {enable_x, enable_z, forward_x, forward_z, control_source, ...}

// Canned Cycles
PUT /hal/threading → Execute threading cycle
PUT /hal/threading/generate → Generate G-code for backplot
PUT /hal/turning → Execute turning cycle
PUT /hal/turning/generate → Generate G-code for backplot

// Emergency Controls
PUT /hal/abort → Abort current operation
PUT /hal/estop → Emergency stop
PUT /hal/cleanup → Clean up after cycles
```

**Electron IPC Channels:**
```typescript
// HAL Management
send('startHAL') → Start LinuxCNC
send('stopHAL') → Stop LinuxCNC
receive('halStarted') → HAL ready signal
receive('halStopped') → HAL shutdown signal

// Settings Persistence
invoke('getSettings') → Load all settings
invoke('saveSettings', data) → Save all settings
```

**Key Data Structures:**
```typescript
interface Tool {
  id: number
  offsetX: number  // X-axis tool offset
  offsetZ: number  // Z-axis tool offset  
  description: string
}

interface HalIn {
  position_z: number
  position_x: number
  position_a: number  // Spindle angle
  speed_rps: number   // Spindle speed
  program_running: boolean
  error_state: boolean
}
```

### State Management Architecture

**Composable-Based State (No Vuex/Pinia — all singletons with module-level state):**
- **useHAL()**: positions, poll loop, feed/direction modes, pitch labels, HAL commands
- **useSettings()**: persisted settings (schema-driven), unit toggle
- **useCannedCycles()**: threading/turning parameters, validation, presets
  (preset dialogs are setup-scoped; call useCannedCycles() from setup)
- **useNumpadEntry()**: the numpad entry state machine + EntryType enum
- **useAppState()**: active screen (MenuType/selectedMenu)
- **usePitchDialog()**: pitch preset dialog (setup-scoped)
- **useToolTable()**: tool selection + showToolTable dialog state
API access goes through `src/api/client.ts` (typed, generated types in
`src/api/types.gen.ts` — regenerate with `yarn generate:api`).

**Position Calculation System:**
```typescript
// Work coordinates = Machine coordinates - Axis offsets + Tool offsets
displayPosition = machinePosition - axisOffset + toolOffset

// Diameter mode: X display = X radius × 2
displayXPos = diameterMode.value ? xpos.value * 2 : xpos.value

// Tool offset application (in useHAL startPoll):
zpos.value = (halIn as any).position_z - zaxisoffset + toolOffsets.currentToolOffsetZ.value
xpos.value = (halIn as any).position_x - xaxisoffset + toolOffsets.currentToolOffsetX.value
```

**Coordinate System Management:**
- Single work coordinate system shared between manual and canned cycle modes
- Axis offsets managed in HAL composable (`xaxisoffset`, `zaxisoffset`, `aaxisoffset`)
- Tool offsets applied separately per tool in position calculations
- Reset position functionality zeroes work coordinates

### Build & Development Workflow

**Key Scripts:**
```bash
# Development
yarn start          # Start frontend dev server + electron app + HAL
yarn serve:front    # Frontend dev server only (port 3000)
yarn watch          # Watch electron main process

# Production  
yarn build          # Compile TypeScript
yarn build:front    # Build Vue frontend
yarn app:build      # Full production build with electron-builder
```

**Development vs Production:**
- **Dev**: Frontend from Vite dev server (localhost:3000), devTools enabled
- **Prod**: Frontend from `file://` protocol, devTools disabled
- **HAL URLs**: Auto-detect Electron vs browser for remote development

### Common Coding Patterns

**Component Pattern:**
```vue
<script setup lang="ts">
// 1. Composable imports at top
import { useHAL } from './composables/useHAL'
import { useSettings } from './composables/useSettings'

// 2. Destructure reactive refs
const { xpos, zpos, startPoll } = useHAL()
const { metric, diameterMode } = useSettings()

// 3. Computed properties for display
const displayValue = computed(() => 
  metric.value ? value : value / 25.4
)
</script>
```

**Unit Conversion Pattern:**
```typescript
// Consistent conversion factor usage
const conversionFactor = toMetric ? 25.4 : 1 / 25.4
value = roundParameterValue(value, conversionFactor)

// Display formatting
const formatValue = (value: number) => 
  metric.value ? value.toFixed(3) : (value / 25.4).toFixed(4)
```

**Error Handling Pattern:**
```typescript
// HAL API calls - graceful degradation
try {
  const response = await fetch(url, options)
  const result = await response.json()
  return result
} catch {
  return {} // Silent failure, return empty object
}
```

### Adding New Features - Standard Process

1. **Add state to appropriate composable** (useHAL, useSettings, useCannedCycles)
2. **Add persistence** if needed: add ONE field to `SettingsSchema` in
   `elle-electron/settings-schema.ts` — nothing else
3. **Add validation functions** for user inputs
4. **Add unit conversion support** for metric/imperial display
5. **Create UI component** following existing PrimeVue patterns
6. **Add to main App.vue** with proper event handling

### Critical File Locations

**Frontend:**
- `elle-frontend/src/App.vue` - Thin shell (menu, status, lifecycle)
- `elle-frontend/src/components/` - Screens + widgets
- `elle-frontend/src/composables/` - Singleton state (see above)
- `elle-frontend/src/api/` - Typed REST client + generated contract types

**Electron:**
- `elle-electron/main.ts` - Main process, IPC handlers
- `elle-electron/settings-schema.ts` - THE settings schema (single source of truth)
- `elle-electron/preload.d.ts` - TypeScript definitions

**Backend:**
- `elle-hal/lathe_halcomp.py` - Flask REST ↔ HAL bridge
- `elle-hal/api_models.py` - REST contract (pydantic, generates TS types)
- `elle-hal/gcode_gen.py` - Pure G-code generators (golden-tested)
- `elle-hal/lathe.ini` - LinuxCNC configuration

### Debugging Tips

**Settings Issues:**
- Check `~/.config/elle-app/appConfig.json` for saved data
- Console errors about "object could not be cloned" = serialization issue
- Look for Vue refs passed without `.value`

**HAL Communication Issues:**
- Check Python backend is running on port 8000
- Verify HAL pins are created: `halcmd show pin`
- Check for 400 errors in network tab = invalid HAL requests

**Position Calculation Issues:**
- Tool offsets applied in `useHAL.ts` `startPoll` function
- Axis offsets managed separately from tool offsets
- Diameter mode doubles X display only, not actual position
