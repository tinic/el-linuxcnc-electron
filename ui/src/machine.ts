// The panel's single store: the latest machine status and settings as reported by the machine,
// plus purely presentational state (units shown, pitch preset labels). The machine is the source
// of truth for everything else; the panel only sends commands.

import { reactive, readonly } from 'vue'

import { createTransport } from './bridge'
import type {
  CyclePlan,
  FromMachine,
  MachineStatus,
  Preferences,
  Settings,
  ToMachine,
} from './protocol'

export type Page = 'manual' | 'cycles' | 'log' | 'settings'

interface PanelState {
  connected: boolean
  simulated: boolean
  status: MachineStatus | null
  settings: Settings | null
  log: string[]
  plan: CyclePlan | null
  notice: { level: 'info' | 'error'; text: string } | null
  page: Page
  metric: boolean
  /** What the pitch preset buttons say, e.g. "Fine" or "18 TPI"; "…" when entered by hand. */
  pitchLabels: { x: string; z: string }
  /** X pitch tracks the Z pitch at this taper angle (degrees), or null for a fixed X pitch. */
  xPitchAngle: number | null
}

const maxLogLines = 2000

const transport = createTransport()

const state = reactive<PanelState>({
  connected: false,
  simulated: transport.simulated,
  status: null,
  settings: null,
  log: [],
  plan: null,
  notice: null,
  page: 'manual',
  metric: true,
  pitchLabels: { x: '…', z: '…' },
  xPitchAngle: null,
})

let receivedSettings = false

function receive(message: FromMachine): void {
  switch (message.type) {
    case 'status':
      state.status = message.status
      state.connected = true
      break
    case 'settings':
      state.settings = message.settings
      if (!receivedSettings) {
        receivedSettings = true
        state.metric = message.settings.metric_on_startup
      }
      break
    case 'log':
      state.log.push(...message.lines)
      if (state.log.length > maxLogLines) {
        state.log.splice(0, state.log.length - maxLogLines)
      }
      break
    case 'plan':
      state.plan = message.plan
      break
    case 'notice':
      state.notice = { level: message.level, text: message.text }
      break
  }
}

transport.listen(receive)

export const panel = readonly(state)

export function send(message: ToMachine): void {
  transport.send(message)
}

export function start(): void {
  send({ type: 'ready' })
  send({ type: 'select_mode', mode: 'manual' })
}

export function showPage(page: Page): void {
  state.page = page
  const mode = page === 'manual' ? 'manual' : page === 'cycles' ? 'cycle' : 'other'
  send({ type: 'select_mode', mode })
}

export function toggleUnits(): void {
  state.metric = !state.metric
}

function taperFeed(angleDeg: number, zPitch: number): number {
  return Math.abs(zPitch * Math.tan((angleDeg * Math.PI) / 180))
}

function sendPitches(x: number, z: number): void {
  send({ type: 'set_pitches', pitches: { x: Math.abs(x), z: Math.abs(z) } })
}

/**
 * Sets one leadscrew pitch (mm/rev). A hand-entered value shows "…" instead of a stale preset
 * name. While X follows a taper angle, a new Z pitch re-derives X so the taper is kept.
 */
export function setPitch(axis: 'x' | 'z', mm: number, label = '…'): void {
  const current = state.status?.pitches ?? { x: 0, z: 0 }
  state.pitchLabels[axis] = label
  if (axis === 'x') {
    state.xPitchAngle = null
    sendPitches(mm, current.z)
    return
  }
  const x = state.xPitchAngle === null ? current.x : taperFeed(state.xPitchAngle, mm)
  sendPitches(x, mm)
}

/** Makes X feed at `angleDeg` relative to Z (taper turning with both slides). */
export function setXTaper(angleDeg: number, label: string): void {
  const current = state.status?.pitches ?? { x: 0, z: 0 }
  state.pitchLabels.x = label
  state.xPitchAngle = angleDeg
  sendPitches(taperFeed(angleDeg, current.z), current.z)
}

export function clearPlan(): void {
  state.plan = null
}

export function dismissNotice(): void {
  state.notice = null
}

export function updatePreferences(change: Partial<Preferences>): void {
  const settings = state.settings
  if (!settings) {
    return
  }
  send({
    type: 'set_preferences',
    preferences: {
      diameter_mode: settings.diameter_mode,
      metric_on_startup: settings.metric_on_startup,
      thread_preset_tab: settings.thread_preset_tab,
      turning_preset_tab: settings.turning_preset_tab,
      pitch_preset_tabs: [...settings.pitch_preset_tabs],
      encoder_scale_z: settings.machine.encoder_scale_z,
      encoder_scale_x: settings.machine.encoder_scale_x,
      back_tool_post: settings.back_tool_post,
      ...change,
    },
  })
}
