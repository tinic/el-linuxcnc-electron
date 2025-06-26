import { ref, defineAsyncComponent } from 'vue'
import { useDialog } from 'primevue/usedialog'

export enum CycleType {
  threading = 'threading',
  peckDrilling = 'peck-drilling',
  turning = 'turning',
}

export enum ThreadingEntryType {
  threadPitch = 6,
  threadXDepth = 7,
  threadZDepth = 8,
  threadAngle = 9,
  threadZEnd = 10,
  threadXPullout = 11,
  threadZPullout = 12,
  threadFirstCut = 13,
  threadCutMult = 14,
  threadMinCut = 15,
  threadSpringCuts = 16,
}

export enum TurningEntryType {
  turningTarget = 17,
  turningZEnd = 20,
  turningFeedRate = 21,
  turningStepDown = 22,
  turningFinalStepDown = 23,
  turningTaperAngle = 24,
  turningSpringPasses = 25,
}

export function useCannedCycles() {
  const dialog = useDialog()

  // Threading parameters
  const threadPitch = ref<number | null>(null)
  const threadXDepth = ref<number | null>(null)
  const threadZDepth = ref<number | null>(null)
  const threadAngle = ref<number | null>(null)
  const threadZEnd = ref<number | null>(null)
  const threadXPullout = ref<number | null>(null)
  const threadZPullout = ref<number | null>(null)
  const threadFirstCut = ref<number | null>(null)
  const threadCutMult = ref<number | null>(null)
  const threadMinCut = ref<number | null>(null)
  const threadSpringCuts = ref<number | null>(null)
  const threadPresetName = ref<string | null>(null)
  const threadDiameter = ref<string | null>(null)

  // Turning parameters
  const turningTarget = ref<number | null>(null)
  const turningZEnd = ref<number | null>(null)
  const turningFeedRate = ref<number | null>(null)
  const turningStepDown = ref<number | null>(null)
  const turningFinalStepDown = ref<number | null>(null)
  const turningTaperAngle = ref<number | null>(null)
  const turningSpringPasses = ref<number | null>(null)
  const turningPresetName = ref<string | null>(null)

  // Future cycle parameters
  const drillDepth = ref<number | null>(null)
  const drillPeckDepth = ref<number | null>(null)
  const drillRetractHeight = ref<number | null>(null)

  // UI state
  const threadingPopovers = ref<Record<string, any>>({})
  const turningPopovers = ref<Record<string, any>>({})
  const currentPopoverLabel = ref('')

  // Threading parameter descriptions
  const threadingDescriptions: { [key: string]: string } = {
    P: 'Thread pitch - distance between threads',
    D: 'Major Diameter / Tap Drill Size\n(major thread diameter and recommended drill size)',
    XD: 'X Depth - cross-slide cutting depth\n(negative for external, positive for internal)',
    ZD: 'Z Depth - longitudinal cutting depth\n(zero for straight threads)',
    A: 'Angle - thread taper angle in degrees\n(zero for straight threads)',
    ZE: 'Z End - final Z position\n(usually negative for regular right hand threads)',
    XP: 'X Pullout - cross-slide retract distance\n(positive for external, negative for internal)',
    ZP: 'Z Pullout - spindle retract distance',
    FC: 'First Cut - initial cutting depth',
    CM: 'Cut Multiplier - depth reduction factor\n(0.5-1.0)',
    MC: 'Min Cut - minimum cutting depth',
    SC: 'Spring Cuts - number of finishing passes'
  }

  // Turning parameter descriptions
  const turningDescriptions: { [key: string]: string } = {
    XS: 'Target - final target diameter\n(desired finished diameter)',
    ZS: 'Z Start - starting Z position\n(current tool position)',
    ZE: 'Length - cut length into material\n(negative for cutting into stock)',
    F: 'Feed Rate - cutting feed per revolution\n(mm/rev or in/rev)',
    SD: 'Step Down - depth per roughing pass\n(material removed per pass)',
    FS: 'Final Step Down - depth of final pass\n(finishing cut depth)',
    TA: 'Taper Angle - angle of taper cut\n(0° for straight turning)',
    SP: 'Spring Passes - number of spring cuts\n(finishing passes at full depth)'
  }

  // Helper functions
  const roundParameterValue = (value: number, conversionFactor: number = 1): number => {
    const result = Math.round(value * conversionFactor * 1000000) / 1000000
    return parseFloat(result.toFixed(6))
  }

  const formatForLinuxCNC = (value: number): number => parseFloat(value.toFixed(6))

  const showLabelPopover = (event: Event, labelKey: string) => {
    const popover = threadingPopovers.value[labelKey]
    if (!popover) {return}

    if (currentPopoverLabel.value === labelKey && popover.visible) {
      popover.hide()
      currentPopoverLabel.value = ''
      return
    }

    if (currentPopoverLabel.value && threadingPopovers.value[currentPopoverLabel.value]) {
      threadingPopovers.value[currentPopoverLabel.value].hide()
    }

    currentPopoverLabel.value = labelKey
    popover.show(event)
  }

  const showTurningLabelPopover = (event: Event, labelKey: string) => {
    const popover = turningPopovers.value[labelKey]
    if (!popover) {return}

    if (currentPopoverLabel.value === labelKey && popover.visible) {
      popover.hide()
      currentPopoverLabel.value = ''
      return
    }

    if (currentPopoverLabel.value && turningPopovers.value[currentPopoverLabel.value]) {
      turningPopovers.value[currentPopoverLabel.value].hide()
    }

    currentPopoverLabel.value = labelKey
    popover.show(event)
  }

  // Threading validation
  const validateThreadingParameters = (): string[] => {
    const errors = []

    if (threadPitch.value === null || threadZEnd.value === null) {
      errors.push('Missing required parameters. Please set Pitch and Z End values.')
      return errors
    }

    if (threadPitch.value <= 0) {
      errors.push('Pitch must be positive')
    }
    if (threadPitch.value > 10) {
      errors.push('Pitch seems too large (>10mm), please verify')
    }

    if (threadXDepth.value !== null && Math.abs(threadXDepth.value) > 10) {
      errors.push('X Depth seems too large (>10mm), please verify')
    }

    if (threadFirstCut.value !== null && threadFirstCut.value <= 0) {
      errors.push('First Cut must be positive')
    }

    if (threadCutMult.value !== null && (threadCutMult.value < 0.5 || threadCutMult.value > 1.0)) {
      errors.push('Cut Multiplier should be between 0.5 and 1.0')
    }

    if (threadMinCut.value !== null && threadMinCut.value <= 0) {
      errors.push('Min Cut must be positive')
    }

    if (
      threadSpringCuts.value !== null &&
      (threadSpringCuts.value < 0 || threadSpringCuts.value > 10)
    ) {
      errors.push('Spring Cuts should be between 0 and 10')
    }

    return errors
  }

  // Threading G-code generation
  const generateThreadingParams = (
    currentXPos: number,
    currentZPos: number,
    currentAPos: number
  ) => {
    const pitch = threadPitch.value || 0
    const xDepth = threadXDepth.value || 0
    const zDepth = threadZDepth.value || 0
    const angle = threadAngle.value || 0
    const zEnd = threadZEnd.value || 0
    const xPullout = threadXPullout.value || 0.1
    const zPullout = threadZPullout.value || 0.1
    const firstCut = threadFirstCut.value || 0.1
    const cutMult = threadCutMult.value || 0.8
    const minCut = threadMinCut.value || 0.05
    const springCuts = threadSpringCuts.value || 1

    const leadIn = pitch * 4
    const userZStart = 0
    const userZEnd = zEnd
    const actualThreadLength = Math.abs(userZEnd - userZStart + zDepth)
    const xEndOffset = angle !== 0 ? actualThreadLength * Math.tan((angle * Math.PI) / 180) : 0
    const actualZStart = userZStart + leadIn
    const actualXStart = angle !== 0 ? leadIn * Math.tan((angle * Math.PI) / 180) : 0

    return {
      XPos: formatForLinuxCNC(currentXPos),
      ZPos: formatForLinuxCNC(currentZPos),
      APos: formatForLinuxCNC(currentAPos),
      XStart: formatForLinuxCNC(currentXPos - actualXStart),
      ZStart: formatForLinuxCNC(currentZPos + actualZStart),
      Pitch: formatForLinuxCNC(pitch),
      XDepth: formatForLinuxCNC(xDepth),
      ZDepth: formatForLinuxCNC(zDepth),
      XEnd: formatForLinuxCNC(currentXPos + xEndOffset),
      ZEnd: formatForLinuxCNC(currentZPos + userZEnd),
      XReturn: formatForLinuxCNC(currentXPos),
      ZReturn: formatForLinuxCNC(currentZPos),
      XPullout: formatForLinuxCNC(xPullout),
      ZPullout: formatForLinuxCNC(zPullout),
      FirstCut: formatForLinuxCNC(firstCut),
      CutMult: formatForLinuxCNC(cutMult),
      MinCut: formatForLinuxCNC(minCut),
      SpringCuts: Math.round(springCuts)
    }
  }

  // Threading preset handling
  const openThreadPresetDialog = (metric: boolean, updatePitchCallback: () => void) => {
    const ThreadPresetSelector = defineAsyncComponent(
      () => import('../components/ThreadPresetSelector.vue')
    )
    const _dialogRef = dialog.open(ThreadPresetSelector, {
      props: {
        header: 'Select Threading Preset',
        style: { width: '70vw', height: '90vh' },
        breakpoints: { '960px': '75vw', '640px': '90vw' },
        position: 'top',
        modal: true
      },
      emits: {
        onSelected: (preset: any) => {
          const conversionFactor = metric ? 1 : 1 / 25.4

          threadPitch.value = roundParameterValue(preset.Pitch, conversionFactor)
          threadXDepth.value = roundParameterValue(preset.XDepth, conversionFactor)
          threadZDepth.value = roundParameterValue(preset.ZDepth, conversionFactor)
          threadAngle.value = roundParameterValue(preset.Angle || 0)
          threadZEnd.value = roundParameterValue(preset.ZEnd, conversionFactor)
          threadXPullout.value = roundParameterValue(preset.XPullout, conversionFactor)
          threadZPullout.value = roundParameterValue(preset.ZPullout, conversionFactor)
          threadFirstCut.value = roundParameterValue(preset.FirstCut, conversionFactor)
          threadCutMult.value = roundParameterValue(preset.CutMult)
          threadMinCut.value = roundParameterValue(preset.MinCut, conversionFactor)
          threadSpringCuts.value = roundParameterValue(preset.SpringCuts)
          threadPresetName.value = preset.name
          threadDiameter.value = preset.Diameter || null
          updatePitchCallback()
        }
      },
      templates: {},
      onClose: (_options) => {}
    })
  }

  // Threading reset
  const resetThreadingParameters = () => {
    threadPitch.value = null
    threadXDepth.value = null
    threadZDepth.value = null
    threadAngle.value = null
    threadZEnd.value = null
    threadXPullout.value = null
    threadZPullout.value = null
    threadFirstCut.value = null
    threadCutMult.value = null
    threadMinCut.value = null
    threadSpringCuts.value = null
    threadPresetName.value = null
    threadDiameter.value = null
  }

  // Threading unit conversion
  const convertThreadingParameters = (toMetric: boolean) => {
    const conversionFactor = toMetric ? 25.4 : 1 / 25.4

    if (threadPitch.value !== null)
    {threadPitch.value = roundParameterValue(threadPitch.value, conversionFactor)}
    if (threadXDepth.value !== null)
    {threadXDepth.value = roundParameterValue(threadXDepth.value, conversionFactor)}
    if (threadZDepth.value !== null)
    {threadZDepth.value = roundParameterValue(threadZDepth.value, conversionFactor)}
    if (threadZEnd.value !== null)
    {threadZEnd.value = roundParameterValue(threadZEnd.value, conversionFactor)}
    if (threadXPullout.value !== null)
    {threadXPullout.value = roundParameterValue(threadXPullout.value, conversionFactor)}
    if (threadZPullout.value !== null)
    {threadZPullout.value = roundParameterValue(threadZPullout.value, conversionFactor)}
    if (threadFirstCut.value !== null)
    {threadFirstCut.value = roundParameterValue(threadFirstCut.value, conversionFactor)}
    if (threadMinCut.value !== null)
    {threadMinCut.value = roundParameterValue(threadMinCut.value, conversionFactor)}

    threadPresetName.value = null
    threadDiameter.value = null
  }

  // Input handling for threading parameters
  const setThreadingParameter = (entryType: ThreadingEntryType, value: number) => {
    const roundedValue = roundParameterValue(value)

    switch (entryType) {
      case ThreadingEntryType.threadPitch:
        threadPitch.value = roundedValue
        break
      case ThreadingEntryType.threadXDepth:
        threadXDepth.value = roundedValue
        break
      case ThreadingEntryType.threadZDepth:
        threadZDepth.value = roundedValue
        break
      case ThreadingEntryType.threadAngle:
        threadAngle.value = roundedValue
        break
      case ThreadingEntryType.threadZEnd:
        threadZEnd.value = roundedValue
        break
      case ThreadingEntryType.threadXPullout:
        threadXPullout.value = roundedValue
        break
      case ThreadingEntryType.threadZPullout:
        threadZPullout.value = roundedValue
        break
      case ThreadingEntryType.threadFirstCut:
        threadFirstCut.value = roundedValue
        break
      case ThreadingEntryType.threadCutMult:
        threadCutMult.value = roundedValue
        break
      case ThreadingEntryType.threadMinCut:
        threadMinCut.value = roundedValue
        break
      case ThreadingEntryType.threadSpringCuts:
        threadSpringCuts.value = roundedValue
        break
    }

    threadPresetName.value = null
    threadDiameter.value = null
  }

  const clearThreadingParameter = (entryType: ThreadingEntryType) => {
    switch (entryType) {
      case ThreadingEntryType.threadPitch:
        threadPitch.value = null
        break
      case ThreadingEntryType.threadXDepth:
        threadXDepth.value = null
        break
      case ThreadingEntryType.threadZDepth:
        threadZDepth.value = null
        break
      case ThreadingEntryType.threadAngle:
        threadAngle.value = null
        break
      case ThreadingEntryType.threadZEnd:
        threadZEnd.value = null
        break
      case ThreadingEntryType.threadXPullout:
        threadXPullout.value = null
        break
      case ThreadingEntryType.threadZPullout:
        threadZPullout.value = null
        break
      case ThreadingEntryType.threadFirstCut:
        threadFirstCut.value = null
        break
      case ThreadingEntryType.threadCutMult:
        threadCutMult.value = null
        break
      case ThreadingEntryType.threadMinCut:
        threadMinCut.value = null
        break
      case ThreadingEntryType.threadSpringCuts:
        threadSpringCuts.value = null
        break
    }
  }

  return {
    // Threading state
    threadPitch,
    threadXDepth,
    threadZDepth,
    threadAngle,
    threadZEnd,
    threadXPullout,
    threadZPullout,
    threadFirstCut,
    threadCutMult,
    threadMinCut,
    threadSpringCuts,
    threadPresetName,
    threadDiameter,

    // Turning state
    turningTarget,
    turningZEnd,
    turningFeedRate,
    turningStepDown,
    turningFinalStepDown,
    turningTaperAngle,
    turningSpringPasses,
    turningPresetName,

    // Future cycle state
    drillDepth,
    drillPeckDepth,
    drillRetractHeight,

    // UI state
    threadingPopovers,
    turningPopovers,
    currentPopoverLabel,
    threadingDescriptions,
    turningDescriptions,

    // Functions
    roundParameterValue,
    formatForLinuxCNC,
    showLabelPopover,
    showTurningLabelPopover,
    validateThreadingParameters,
    generateThreadingParams,
    openThreadPresetDialog,
    resetThreadingParameters,
    convertThreadingParameters,
    setThreadingParameter,
    clearThreadingParameter,

    // Turning functions
    validateTurningParameters,
    generateTurningParams,
    openTurningPresetDialog,
    resetTurningParameters,
    convertTurningParameters,
    setTurningParameter,
    clearTurningParameter
  }

  // Turning validation
  function validateTurningParameters(): string[] {
    const errors = []

    if (turningTarget.value === null || turningZEnd.value === null) {
      errors.push('Missing required parameters. Please set Target and Length values.')
      return errors
    }


    if (turningFeedRate.value !== null && turningFeedRate.value <= 0) {
      errors.push('Feed Rate must be positive')
    }

    if (turningStepDown.value !== null && turningStepDown.value <= 0) {
      errors.push('Step Down must be positive')
    }

    if (turningFinalStepDown.value !== null && turningFinalStepDown.value <= 0) {
      errors.push('Final Step Down must be positive')
    }

    if (
      turningSpringPasses.value !== null &&
      (turningSpringPasses.value < 0 || turningSpringPasses.value > 10)
    ) {
      errors.push('Spring Passes should be between 0 and 10')
    }

    if (turningTaperAngle.value !== null && Math.abs(turningTaperAngle.value) > 45) {
      errors.push('Taper Angle seems too large (>45°), please verify')
    }

    return errors
  }

  // Turning G-code generation
  function generateTurningParams(currentXPos: number, currentZPos: number, currentAPos: number) {
    const stockRadius = currentXPos
    const targetRadius = turningTarget.value || 0
    const zEnd = turningZEnd.value || 0
    const feedRate = turningFeedRate.value || 0.2
    const stepDown = turningStepDown.value || 0.5
    const finalStepDown = turningFinalStepDown.value || 0.1
    const taperAngle = turningTaperAngle.value || 0
    const springPasses = turningSpringPasses.value || 0

    // Feed rate is already in mm/rev, so it's directly the pitch
    const pitch = feedRate

    // Calculate ZLead as a small lead-in distance (typically 2mm)
    const zLead = 2.0

    return {
      XPos: formatForLinuxCNC(currentXPos),
      ZPos: formatForLinuxCNC(currentZPos),
      APos: formatForLinuxCNC(currentAPos),
      Pitch: formatForLinuxCNC(pitch),
      Stock: formatForLinuxCNC(stockRadius),
      Target: formatForLinuxCNC(targetRadius),
      ZLead: formatForLinuxCNC(zLead),
      ZEnd: formatForLinuxCNC(zEnd),
      Angle: formatForLinuxCNC(taperAngle),
      StepDown: formatForLinuxCNC(stepDown),
      FinalStepDown: formatForLinuxCNC(finalStepDown),
      SpringPasses: Math.round(springPasses),
      XReturn: formatForLinuxCNC(targetRadius),
      ZReturn: formatForLinuxCNC(currentZPos)
    }
  } 

  // Turning preset handling
  function openTurningPresetDialog(metric: boolean, updatePitchCallback: () => void) {
    const TurningPresetSelector = defineAsyncComponent(
      () => import('../components/TurningPresetSelector.vue')
    )
    const _dialogRef = dialog.open(TurningPresetSelector, {
      props: {
        header: 'Select Turning Preset',
        style: { width: '70vw', height: '90vh' },
        breakpoints: { '960px': '75vw', '640px': '90vw' },
        position: 'top',
        modal: true
      },
      emits: {
        onSelected: (preset: any) => {
          const conversionFactor = metric ? 1 : 1 / 25.4

          turningTarget.value = roundParameterValue(preset.target, conversionFactor)
          turningZEnd.value = roundParameterValue(preset.zEnd, conversionFactor)
          turningFeedRate.value = roundParameterValue(preset.feedRate, conversionFactor)
          turningStepDown.value = roundParameterValue(preset.stepDown, conversionFactor)
          turningFinalStepDown.value = roundParameterValue(preset.finalStepDown, conversionFactor)
          turningTaperAngle.value = roundParameterValue(preset.taperAngle)
          turningSpringPasses.value = roundParameterValue(preset.springPasses || 0)
          turningPresetName.value = preset.name
          updatePitchCallback()
        }
      },
      templates: {},
      onClose: (_options) => {}
    })
  }

  // Turning reset
  function resetTurningParameters() {
    turningTarget.value = null
    turningZEnd.value = null
    turningFeedRate.value = null
    turningStepDown.value = null
    turningFinalStepDown.value = null
    turningTaperAngle.value = null
    turningSpringPasses.value = null
    turningPresetName.value = null
  }

  // Turning unit conversion
  function convertTurningParameters(toMetric: boolean) {
    const conversionFactor = toMetric ? 25.4 : 1 / 25.4

    if (turningTarget.value !== null)
    {turningTarget.value = roundParameterValue(turningTarget.value, conversionFactor)}
    if (turningZEnd.value !== null)
    {turningZEnd.value = roundParameterValue(turningZEnd.value, conversionFactor)}
    if (turningFeedRate.value !== null)
    {turningFeedRate.value = roundParameterValue(turningFeedRate.value, conversionFactor)}
    if (turningStepDown.value !== null)
    {turningStepDown.value = roundParameterValue(turningStepDown.value, conversionFactor)}
    if (turningFinalStepDown.value !== null)
    {turningFinalStepDown.value = roundParameterValue(turningFinalStepDown.value, conversionFactor)}

    turningPresetName.value = null
  }

  // Input handling for turning parameters
  function setTurningParameter(entryType: TurningEntryType, value: number) {
    const roundedValue = roundParameterValue(value)

    switch (entryType) {
      case TurningEntryType.turningTarget:
        turningTarget.value = roundedValue
        break
      case TurningEntryType.turningZEnd:
        turningZEnd.value = roundedValue
        break
      case TurningEntryType.turningFeedRate:
        turningFeedRate.value = roundedValue
        break
      case TurningEntryType.turningStepDown:
        turningStepDown.value = roundedValue
        break
      case TurningEntryType.turningFinalStepDown:
        turningFinalStepDown.value = roundedValue
        break
      case TurningEntryType.turningTaperAngle:
        turningTaperAngle.value = roundedValue
        break
      case TurningEntryType.turningSpringPasses:
        turningSpringPasses.value = roundedValue
        break
    }

    turningPresetName.value = null
  }

  function clearTurningParameter(entryType: TurningEntryType) {
    switch (entryType) {
      case TurningEntryType.turningTarget:
        turningTarget.value = null
        break
      case TurningEntryType.turningZEnd:
        turningZEnd.value = null
        break
      case TurningEntryType.turningFeedRate:
        turningFeedRate.value = null
        break
      case TurningEntryType.turningStepDown:
        turningStepDown.value = null
        break
      case TurningEntryType.turningFinalStepDown:
        turningFinalStepDown.value = null
        break
      case TurningEntryType.turningTaperAngle:
        turningTaperAngle.value = null
        break
      case TurningEntryType.turningSpringPasses:
        turningSpringPasses.value = null
        break
    }
  }
}
