import { computed, ref, watch } from 'vue'
import { api, useConnection } from '../api/client'
import type {
  CleanupResponse,
  GcodeResponse,
  HalOut,
  StatusResponse,
  ThreadingExecuteParams,
  ThreadingParams,
  TurningExecuteParams,
  TurningParams
} from '../api/types.gen'
import { useSettings } from './useSettings'
import { MenuType, selectedMenu } from './useAppState'

function errorEnvelope(error: unknown): StatusResponse {
  return {
    status: 'Error',
    message: error instanceof Error ? error.message : String(error)
  }
}

// Legacy-compatible wrappers: these never throw, so existing call sites can
// keep fire-and-forget semantics. Failures are still visible through
// useConnection() and the returned status envelope instead of a silent {}.
const putHalOut = (halOut: HalOut): Promise<StatusResponse> =>
  api.putHalOut(halOut).catch(errorEnvelope)
const putAbort = (): Promise<StatusResponse> =>
  api.abort().catch(errorEnvelope)
const putEmergencyStop = (): Promise<StatusResponse> =>
  api.emergencyStop().catch(errorEnvelope)
const putThreading = (params: ThreadingExecuteParams): Promise<GcodeResponse> =>
  api.executeThreading(params).catch((e) => ({ ...errorEnvelope(e), gcode: [] }))
const generateThreadingGcode = (params: ThreadingParams): Promise<GcodeResponse> =>
  api.generateThreadingGcode(params).catch((e) => ({ ...errorEnvelope(e), gcode: [] }))
const putTurning = (params: TurningExecuteParams): Promise<GcodeResponse> =>
  api.executeTurning(params).catch((e) => ({ ...errorEnvelope(e), gcode: [] }))
const generateTurningGcode = (params: TurningParams): Promise<GcodeResponse> =>
  api.generateTurningGcode(params).catch((e) => ({ ...errorEnvelope(e), gcode: [] }))
const cleanupCannedCycles = (): Promise<CleanupResponse> =>
  api.cleanupCannedCycles().catch((e) => ({ ...errorEnvelope(e), files_removed: [] }))
const getHalIn = api.getHalIn
const putLinuxCNC = api.putLinuxCNC

export enum FeedMode {
  none = 0,
  longitudinal = 1,
  cross = 2,
  frontCompound = 3,
  backCompound = 4
}

export enum DirectionMode {
  none = 0,
  forward = 1,
  reverse = 2,
  hold = 3,
  idle = 4
}

// Settings are singleton state; grab the refs the HAL layer needs.
const {
  encoderScaleZ,
  encoderScaleX,
  diameterMode,
  pitchX,
  pitchZ,
  currentToolOffsetX,
  currentToolOffsetZ
} = useSettings()

// --- Singleton machine state (shared by every consumer of useHAL) ---

const xpos = ref(0)
const zpos = ref(0)
const apos = ref(0)
const rpms = ref(0)
const rpmsSmoothed = ref(0)
const cannedCycleRunning = ref(false)
const errorState = ref(false)
const xpitch = ref(0.1)
const zpitch = ref(0.1)
const xpitchactive = ref(false)
const zpitchactive = ref(false)
const xstepperactive = ref(false)
const zstepperactive = ref(false)

const selectedFeedMode = ref(FeedMode.longitudinal)
const selectedDirectionMode = ref(DirectionMode.forward)

// DRO pitch labels (what the pitch cells display: preset name, TPI, angle…)
const xpitchlabel = ref('…')
const zpitchlabel = ref('…')
const xpitchangle = ref(0)

// Display positions: diameter mode doubles X only; Z is always linear.
const displayXPos = computed(() => (diameterMode.value ? xpos.value * 2 : xpos.value))
const displayZPos = computed(() => zpos.value)

export function pitchForAngle(pitch: number, angle: number) {
  return pitch * Math.tan(angle * (Math.PI / 180))
}

let updateInterval: ReturnType<typeof setInterval>
let halOutResetPositionScheduled: boolean = false
let halOutScheduled: boolean = false
let xaxisoffset: number = 0
let zaxisoffset: number = 0
let aaxisoffset: number = 0
let xaxisset: number = 0
let zaxisset: number = 0
let aaxisset: number = 0
let xaxissetscheduled: boolean = false
let zaxissetscheduled: boolean = false
let aaxissetscheduled: boolean = false
let buttonuptime: number = 0
let buttondowntime: number = 0
let buttonlefttime: number = 0
let buttonrighttime: number = 0
let buttonupscheduled: boolean = false
let zforward: boolean = true
let xforward: boolean = true

function stopJogNow() {
  buttonuptime = 0
  buttondowntime = 0
  buttonlefttime = 0
  buttonrighttime = 0
  const halOut = {
    control_stop_now: 1
  }
  putHalOut(halOut)
}

function startPoll() {
  updateInterval = setInterval(() => {
    if (halOutResetPositionScheduled) {
      halOutResetPositionScheduled = false
      xaxisoffset = -xpos.value - currentToolOffsetX.value
      zaxisoffset = -zpos.value - currentToolOffsetZ.value
      const halOut = {
        reset_position: true,
        encoder_scale_z: encoderScaleZ.value,
        encoder_scale_x: encoderScaleX.value
      }
      putHalOut(halOut)
    }
    api
      .getHalIn()
      .then((halIn) => {
        if (xaxissetscheduled) {
          xaxissetscheduled = false
          xaxisoffset = halIn.position_x - xaxisset
          xaxisset = 0
        }
        if (zaxissetscheduled) {
          zaxissetscheduled = false
          zaxisoffset = halIn.position_z - zaxisset
          zaxisset = 0
        }
        if (aaxissetscheduled) {
          aaxissetscheduled = false
          aaxisoffset = halIn.position_a - ((aaxisset / 360) % 1)
          aaxisset = 0
        }
        zpos.value = halIn.position_z - zaxisoffset + currentToolOffsetZ.value
        xpos.value = halIn.position_x - xaxisoffset + currentToolOffsetX.value
        apos.value = Math.abs(((halIn.position_a - aaxisoffset) % 1) * 360)
        const newRpm = Math.abs(halIn.speed_rps * 60)
        rpms.value = newRpm
        // Apply exponential smoothing filter (alpha = 0.2 for dampening)
        rpmsSmoothed.value = rpmsSmoothed.value * 0.8 + newRpm * 0.2
        cannedCycleRunning.value = halIn.program_running || false
        errorState.value = halIn.error_state || false
      })
      .catch(() => {
        // Connection loss is tracked by the API client (useConnection);
        // position refs simply keep their last known values.
      })
    if (buttonuptime > 0) {
      halOutScheduled = false
      let velocity = (Date.now() / 1000 - buttonuptime) * 3
      velocity = Math.min(velocity, 3.0)
      const halOut = {
        control_x_type: 1,
        velocity_x_cmd: -velocity
      }
      putHalOut(halOut)
    }
    if (buttondowntime > 0) {
      halOutScheduled = false
      let velocity = (Date.now() / 1000 - buttondowntime) * 3
      velocity = Math.min(velocity, 3.0)
      const halOut = {
        control_x_type: 1,
        velocity_x_cmd: +velocity
      }
      putHalOut(halOut)
    }
    if (buttonlefttime > 0) {
      halOutScheduled = false
      let velocity = (Date.now() / 1000 - buttonlefttime) * 3
      velocity = Math.min(velocity, 6.0)
      const halOut = {
        control_z_type: 1,
        velocity_z_cmd: -velocity
      }
      putHalOut(halOut)
    }
    if (buttonrighttime > 0) {
      halOutScheduled = false
      let velocity = (Date.now() / 1000 - buttonrighttime) * 3
      velocity = Math.min(velocity, 6.0)
      const halOut = {
        control_z_type: 1,
        velocity_z_cmd: +velocity
      }
      putHalOut(halOut)
    }
    if (buttonupscheduled) {
      buttonupscheduled = false
      stopJogNow()
    }
    if (halOutScheduled) {
      halOutScheduled = false
      if (selectedMenu.value === MenuType.manual) {
        const halOut = {
          control_source: false,
          forward_z: zforward ? -zpitch.value : +zpitch.value,
          forward_x: xforward ? +xpitch.value : -xpitch.value,
          enable_z: zpitchactive.value,
          enable_x: xpitchactive.value,
          enable_stepper_z: zstepperactive.value,
          enable_stepper_x: xstepperactive.value,
          encoder_scale_z: encoderScaleZ.value,
          encoder_scale_x: encoderScaleX.value
        }
        putHalOut(halOut)
      } else if (selectedMenu.value === MenuType.cannedCycles) {
        selectedDirectionMode.value = DirectionMode.hold
        selectedFeedMode.value = FeedMode.backCompound
        const halOut = {
          control_source: true,
          forward_z: zforward ? -zpitch.value : +zpitch.value,
          forward_x: xforward ? +xpitch.value : -xpitch.value,
          enable_z: true,
          enable_x: true,
          enable_stepper_z: true,
          enable_stepper_x: true,
          encoder_scale_z: encoderScaleZ.value,
          encoder_scale_x: encoderScaleX.value
        }
        putHalOut(halOut)
      }
    }
  }, 33.33333)
}

function endPoll() {
  clearTimeout(updateInterval)
}

const setAxisOffset = (axis: 'x' | 'z' | 'a', value: number) => {
  if (axis === 'x') {
    xaxisoffset = value
  } else if (axis === 'z') {
    zaxisoffset = value
  } else if (axis === 'a') {
    aaxisoffset = value
  }
}

const scheduleResetPosition = () => {
  halOutResetPositionScheduled = true
}

const scheduleHALOut = () => {
  halOutScheduled = true
}

const setButtonTime = (button: 'up' | 'down' | 'left' | 'right', time: number) => {
  if (button === 'up') {
    buttonuptime = time
  } else if (button === 'down') {
    buttondowntime = time
  } else if (button === 'left') {
    buttonlefttime = time
  } else if (button === 'right') {
    buttonrighttime = time
  }
}

const scheduleButtonUp = () => {
  buttonupscheduled = true
}

const getAxisOffset = (axis: 'x' | 'z' | 'a') => {
  if (axis === 'x') {
    return xaxisoffset
  } else if (axis === 'z') {
    return zaxisoffset
  } else if (axis === 'a') {
    return aaxisoffset
  }
  return 0
}

const setAxisValue = (axis: 'x' | 'z' | 'a', value: number) => {
  if (axis === 'x') {
    xaxisset = value
    xaxissetscheduled = true
  } else if (axis === 'z') {
    zaxisset = value
    zaxissetscheduled = true
  } else if (axis === 'a') {
    aaxisset = value
    aaxissetscheduled = true
  }
}

const updateHALOut = () => {
  switch (selectedFeedMode.value) {
    case FeedMode.longitudinal:
      switch (selectedDirectionMode.value) {
        case DirectionMode.forward:
          zstepperactive.value = true
          xstepperactive.value = false
          zpitchactive.value = true
          xpitchactive.value = false
          zforward = true
          xforward = true
          break
        case DirectionMode.reverse:
          zstepperactive.value = true
          xstepperactive.value = false
          zpitchactive.value = true
          xpitchactive.value = false
          zforward = false
          xforward = false
          break
        case DirectionMode.hold:
          zstepperactive.value = false
          xstepperactive.value = false
          zpitchactive.value = true
          xpitchactive.value = false
          zforward = true
          xforward = true
          break
        case DirectionMode.idle:
          zstepperactive.value = false
          xstepperactive.value = false
          zpitchactive.value = false
          xpitchactive.value = false
          zforward = true
          xforward = true
          break
      }
      break
    case FeedMode.cross:
      switch (selectedDirectionMode.value) {
        case DirectionMode.forward:
          zstepperactive.value = false
          xstepperactive.value = true
          zpitchactive.value = false
          xpitchactive.value = true
          zforward = true
          xforward = false
          break
        case DirectionMode.reverse:
          zstepperactive.value = false
          xstepperactive.value = true
          zpitchactive.value = false
          xpitchactive.value = true
          zforward = false
          xforward = true
          break
        case DirectionMode.hold:
          zstepperactive.value = false
          xstepperactive.value = false
          zpitchactive.value = false
          xpitchactive.value = true
          zforward = true
          xforward = true
          break
        case DirectionMode.idle:
          zstepperactive.value = false
          xstepperactive.value = false
          zpitchactive.value = false
          xpitchactive.value = false
          zforward = true
          xforward = true
          break
      }
      break
    case FeedMode.frontCompound:
      switch (selectedDirectionMode.value) {
        case DirectionMode.forward:
          zstepperactive.value = true
          xstepperactive.value = true
          zpitchactive.value = true
          xpitchactive.value = true
          zforward = true
          xforward = true
          break
        case DirectionMode.reverse:
          zstepperactive.value = true
          xstepperactive.value = true
          zpitchactive.value = true
          xpitchactive.value = true
          zforward = false
          xforward = false
          break
        case DirectionMode.hold:
          zstepperactive.value = false
          xstepperactive.value = false
          zpitchactive.value = true
          xpitchactive.value = true
          zforward = true
          xforward = true
          break
        case DirectionMode.idle:
          zstepperactive.value = false
          xstepperactive.value = false
          zpitchactive.value = false
          xpitchactive.value = false
          zforward = true
          xforward = true
          break
      }
      break
    case FeedMode.backCompound:
      switch (selectedDirectionMode.value) {
        case DirectionMode.forward:
          zstepperactive.value = true
          xstepperactive.value = true
          zpitchactive.value = true
          xpitchactive.value = true
          zforward = true
          xforward = false
          break
        case DirectionMode.reverse:
          zstepperactive.value = true
          xstepperactive.value = true
          zpitchactive.value = true
          xpitchactive.value = true
          zforward = false
          xforward = true
          break
        case DirectionMode.hold:
          zstepperactive.value = false
          xstepperactive.value = false
          zpitchactive.value = true
          xpitchactive.value = true
          zforward = true
          xforward = false
          break
        case DirectionMode.idle:
          zstepperactive.value = false
          xstepperactive.value = false
          zpitchactive.value = false
          xpitchactive.value = false
          zforward = true
          xforward = false
          break
      }
      break
  }
  scheduleHALOut()
}

// Feed/direction mode changes and pitch changes push a fresh HAL command;
// pitch values also persist into settings.
watch([selectedFeedMode, selectedDirectionMode], () => {
  updateHALOut()
})

watch([zpitch, xpitch], () => {
  updateHALOut()
  pitchX.value = xpitch.value
  pitchZ.value = zpitch.value
})

// Switching screens re-anchors work coordinates and re-sends control state.
watch(selectedMenu, () => {
  scheduleResetPosition()
  scheduleHALOut()
})

export function useHAL() {
  const { connectionState, isConnected, lastError } = useConnection()

  return {
    connectionState,
    isConnected,
    lastError,
    xpos,
    zpos,
    apos,
    rpms,
    rpmsSmoothed,
    cannedCycleRunning,
    errorState,
    xpitch,
    zpitch,
    xpitchactive,
    zpitchactive,
    xstepperactive,
    zstepperactive,
    selectedFeedMode,
    selectedDirectionMode,
    xpitchlabel,
    zpitchlabel,
    xpitchangle,
    displayXPos,
    displayZPos,
    putHalOut,
    putLinuxCNC,
    getHalIn,
    putAbort,
    putEmergencyStop,
    putThreading,
    generateThreadingGcode,
    putTurning,
    generateTurningGcode,
    cleanupCannedCycles,
    startPoll,
    endPoll,
    stopJogNow,
    setAxisOffset,
    scheduleResetPosition,
    scheduleHALOut,
    setButtonTime,
    scheduleButtonUp,
    getAxisOffset,
    setAxisValue,
    updateHALOut
  }
}
