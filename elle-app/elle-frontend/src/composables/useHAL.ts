import { ref } from 'vue'
import {
  putHalOut,
  putLinuxCNC,
  getHalIn,
  putAbort,
  putEmergencyStop,
  putThreading,
  generateThreadingGcode,
  putTurning,
  generateTurningGcode,
  cleanupCannedCycles
} from '../HAL'

export function useHAL() {
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

  let updateInterval: NodeJS.Timeout
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

  function startPoll(toolOffsets: { currentToolOffsetX: any, currentToolOffsetZ: any }, params: {
    selectedMenu: any
    MenuType: any
    DirectionMode: any
    FeedMode: any
    selectedDirectionMode: any
    selectedFeedMode: any
  }) {
    updateInterval = setInterval(() => {
      if (halOutResetPositionScheduled) {
        halOutResetPositionScheduled = false
        xaxisoffset = -xpos.value - toolOffsets.currentToolOffsetX.value
        zaxisoffset = -zpos.value - toolOffsets.currentToolOffsetZ.value
        const halOut = {
          reset_position: true
        }
        putHalOut(halOut)
      }
      try {
        getHalIn().then((halIn) => {
          if (xaxissetscheduled) {
            xaxissetscheduled = false
            xaxisoffset = (halIn as any).position_x - xaxisset
            xaxisset = 0
          }
          if (zaxissetscheduled) {
            zaxissetscheduled = false
            zaxisoffset = (halIn as any).position_z - zaxisset
            zaxisset = 0
          }
          if (aaxissetscheduled) {
            aaxissetscheduled = false
            aaxisoffset = (halIn as any).position_a - ((aaxisset / 360) % 1)
            aaxisset = 0
          }
          zpos.value = (halIn as any).position_z - zaxisoffset + toolOffsets.currentToolOffsetZ.value
          xpos.value = (halIn as any).position_x - xaxisoffset + toolOffsets.currentToolOffsetX.value
          apos.value = Math.abs((((halIn as any).position_a - aaxisoffset) % 1) * 360)
          const newRpm = Math.abs((halIn as any).speed_rps * 60)
          rpms.value = newRpm
          // Apply exponential smoothing filter (alpha = 0.2 for dampening)
          rpmsSmoothed.value = rpmsSmoothed.value * 0.8 + newRpm * 0.2
          cannedCycleRunning.value = (halIn as any).program_running || false
          errorState.value = (halIn as any).error_state || false
        })
      } catch {
        // Ignore polling errors
      }
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
        if (params.selectedMenu.value == params.MenuType.manual) {
          const halOut = {
            control_source: false,
            forward_z: zforward ? -zpitch.value : +zpitch.value,
            forward_x: xforward ? +xpitch.value : -xpitch.value,
            enable_z: zpitchactive.value,
            enable_x: xpitchactive.value,
            enable_stepper_z: zstepperactive.value,
            enable_stepper_x: xstepperactive.value
          }
          putHalOut(halOut)
        } else if (params.selectedMenu.value == params.MenuType.cannedCycles) {
          params.selectedDirectionMode.value = params.DirectionMode.hold
          params.selectedFeedMode.value = params.FeedMode.backCompound
          const halOut = {
            control_source: true,
            forward_z: zforward ? -zpitch.value : +zpitch.value,
            forward_x: xforward ? +xpitch.value : -xpitch.value,
            enable_z: true,
            enable_x: true,
            enable_stepper_z: true,
            enable_stepper_x: true
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
    if (axis === 'x') {xaxisoffset = value}
    else if (axis === 'z') {zaxisoffset = value}
    else if (axis === 'a') {aaxisoffset = value}
  }

  const scheduleResetPosition = () => {
    halOutResetPositionScheduled = true
  }

  const scheduleHALOut = () => {
    halOutScheduled = true
  }

  const setButtonTime = (button: 'up' | 'down' | 'left' | 'right', time: number) => {
    if (button === 'up') {buttonuptime = time}
    else if (button === 'down') {buttondowntime = time}
    else if (button === 'left') {buttonlefttime = time}
    else if (button === 'right') {buttonrighttime = time}
  }

  const scheduleButtonUp = () => {
    buttonupscheduled = true
  }

  const getAxisOffset = (axis: 'x' | 'z' | 'a') => {
    if (axis === 'x') {return xaxisoffset}
    else if (axis === 'z') {return zaxisoffset}
    else if (axis === 'a') {return aaxisoffset}
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

  const updateHALOut = (
    selectedFeedMode: any,
    selectedDirectionMode: any,
    FeedMode: any,
    DirectionMode: any
  ) => {
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

  return {
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
