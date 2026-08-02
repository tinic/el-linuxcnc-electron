import { ref } from 'vue'
import { useSettings } from './useSettings'
import { useHAL } from './useHAL'
import {
  ThreadingEntryType,
  TurningEntryType,
  setThreadingParameter,
  clearThreadingParameter,
  setTurningParameter,
  clearTurningParameter,
  updatePitchFromThread,
  updatePitchFromTurning
} from './useCannedCycles'

export enum EntryType {
  xPosition = 1,
  zPosition = 2,
  aPosition = 3,
  xPitch = 4,
  zPitch = 5
}

const { metric, diameterMode } = useSettings()
const { xpos, zpos, apos, xpitch, zpitch, setAxisValue, scheduleHALOut } = useHAL()

const numberentry = ref(0)
const cursorpos = ref(0)

enum NumpadInputStage {
  none = 0,
  start = 1,
  entry = 2
}

const entryActive = ref(0)

let numpadInputStage = NumpadInputStage.none
const numbersClicked = new Array<string>()
let numbersNegative = false
let numbersPrevious: number = 0

function treatOffClickAsEnter() {
  if (numpadInputStage == NumpadInputStage.start) {
    numberentry.value = numbersPrevious
    setFinalNumber(numbersPrevious)
  } else if (numpadInputStage == NumpadInputStage.entry) {
    numberentry.value = calcNumber()
    setFinalNumber(numberentry.value)
  }
}

const numberClicked = (entry: number, value: number) => {
  treatOffClickAsEnter()
  numbersClicked.length = 0
  numpadInputStage = NumpadInputStage.start
  switch (entry) {
    case EntryType.xPosition:
      numberentry.value = numbersPrevious = metric.value ? value : value / 25.4
      break
    case EntryType.zPosition:
    case EntryType.xPitch:
    case EntryType.zPitch:
    case ThreadingEntryType.threadPitch:
    case ThreadingEntryType.threadXDepth:
    case ThreadingEntryType.threadZDepth:
    case ThreadingEntryType.threadAngle:
    case ThreadingEntryType.threadZEnd:
    case ThreadingEntryType.threadXPullout:
    case ThreadingEntryType.threadZPullout:
    case ThreadingEntryType.threadFirstCut:
    case ThreadingEntryType.threadMinCut:
    case TurningEntryType.turningTarget:
    case TurningEntryType.turningZEnd:
    case TurningEntryType.turningFeedRate:
    case TurningEntryType.turningStepDown:
    case TurningEntryType.turningFinalStepDown:
    case TurningEntryType.turningTaperAngle:
      numberentry.value = numbersPrevious = metric.value ? value : value / 25.4
      break
    case EntryType.aPosition:
    case ThreadingEntryType.threadCutMult:
    case ThreadingEntryType.threadSpringCuts:
    case TurningEntryType.turningSpringPasses:
      numberentry.value = numbersPrevious = value
      break
  }
  entryActive.value = entry
  numbersNegative = false
}

function calcNumber(): number {
  const dotIndex = numbersClicked.indexOf('.')
  let integerSize: number
  let fractionSize = 0
  if (dotIndex >= 0) {
    integerSize = dotIndex
    fractionSize = numbersClicked.length - dotIndex - 1
    cursorpos.value = numbersClicked.length - dotIndex + 1
  } else {
    integerSize = numbersClicked.length
    if (numbersClicked.length == 0) {
      cursorpos.value = 0
    } else {
      cursorpos.value = 1
    }
  }
  let value: number = 0
  for (let i = 0; i < integerSize; i++) {
    value += (numbersClicked[i].charCodeAt(0) - 0x30) * Math.pow(10, integerSize - i - 1)
  }
  for (let i = 0; i < fractionSize; i++) {
    value += (numbersClicked[i + integerSize + 1].charCodeAt(0) - 0x30) * Math.pow(10, -i - 1)
  }
  return value * (numbersNegative ? -1 : +1)
}

function setFinalNumber(value: number) {
  switch (entryActive.value) {
    case EntryType.xPosition:
    case EntryType.zPosition:
    case EntryType.xPitch:
    case EntryType.zPitch:
    case ThreadingEntryType.threadPitch:
    case ThreadingEntryType.threadXDepth:
    case ThreadingEntryType.threadZDepth:
    case ThreadingEntryType.threadAngle:
    case ThreadingEntryType.threadZEnd:
    case ThreadingEntryType.threadXPullout:
    case ThreadingEntryType.threadZPullout:
    case ThreadingEntryType.threadFirstCut:
    case ThreadingEntryType.threadMinCut:
    case TurningEntryType.turningTarget:
    case TurningEntryType.turningZEnd:
    case TurningEntryType.turningFeedRate:
    case TurningEntryType.turningStepDown:
    case TurningEntryType.turningFinalStepDown:
    case TurningEntryType.turningTaperAngle:
      if (!metric.value) {
        value = value * 25.4
      }
      break
    case EntryType.aPosition:
    case ThreadingEntryType.threadCutMult:
    case ThreadingEntryType.threadSpringCuts:
    case TurningEntryType.turningSpringPasses:
      break
  }
  switch (entryActive.value) {
    case EntryType.xPosition: {
      const actualXValue = diameterMode.value ? value / 2 : value
      setAxisValue('x', actualXValue)
      xpos.value = actualXValue
      break
    }
    case EntryType.zPosition:
      setAxisValue('z', value)
      zpos.value = value
      break
    case EntryType.aPosition:
      setAxisValue('a', value)
      apos.value = value
      break
    case EntryType.xPitch:
      xpitch.value = Math.abs(value)
      break
    case EntryType.zPitch:
      zpitch.value = Math.abs(value)
      break
    case ThreadingEntryType.threadPitch:
      setThreadingParameter(ThreadingEntryType.threadPitch, value)
      updatePitchFromThread()
      break
    case ThreadingEntryType.threadXDepth:
      setThreadingParameter(ThreadingEntryType.threadXDepth, value)
      break
    case ThreadingEntryType.threadZDepth:
      setThreadingParameter(ThreadingEntryType.threadZDepth, value)
      break
    case ThreadingEntryType.threadAngle:
      setThreadingParameter(ThreadingEntryType.threadAngle, value)
      break
    case ThreadingEntryType.threadZEnd:
      setThreadingParameter(ThreadingEntryType.threadZEnd, value)
      break
    case ThreadingEntryType.threadXPullout:
      setThreadingParameter(ThreadingEntryType.threadXPullout, value)
      break
    case ThreadingEntryType.threadZPullout:
      setThreadingParameter(ThreadingEntryType.threadZPullout, value)
      break
    case ThreadingEntryType.threadFirstCut:
      setThreadingParameter(ThreadingEntryType.threadFirstCut, value)
      break
    case ThreadingEntryType.threadCutMult:
      setThreadingParameter(ThreadingEntryType.threadCutMult, value)
      break
    case ThreadingEntryType.threadMinCut:
      setThreadingParameter(ThreadingEntryType.threadMinCut, value)
      break
    case ThreadingEntryType.threadSpringCuts:
      setThreadingParameter(ThreadingEntryType.threadSpringCuts, value)
      break
    case TurningEntryType.turningTarget: {
      const actualTurningTarget = diameterMode.value ? value / 2 : value
      setTurningParameter(TurningEntryType.turningTarget, actualTurningTarget)
      updatePitchFromTurning()
      break
    }
    case TurningEntryType.turningZEnd:
      setTurningParameter(TurningEntryType.turningZEnd, value)
      break
    case TurningEntryType.turningFeedRate:
      setTurningParameter(TurningEntryType.turningFeedRate, value)
      break
    case TurningEntryType.turningStepDown:
      setTurningParameter(TurningEntryType.turningStepDown, value)
      break
    case TurningEntryType.turningSpringPasses:
      setTurningParameter(TurningEntryType.turningSpringPasses, value)
      break
    case TurningEntryType.turningFinalStepDown:
      setTurningParameter(TurningEntryType.turningFinalStepDown, value)
      break
    case TurningEntryType.turningTaperAngle:
      setTurningParameter(TurningEntryType.turningTaperAngle, value)
      break
  }
  numpadInputStage = NumpadInputStage.none
  numbersClicked.length = 0
  entryActive.value = 0
  cursorpos.value = 0
}

const numPadClicked = (key: string) => {
  if (numpadInputStage == NumpadInputStage.none) {
    return
  }
  switch (key) {
    case 'Escape':
      numpadInputStage = NumpadInputStage.none
      numberentry.value = numbersPrevious
      numbersClicked.length = 0
      entryActive.value = 0
      cursorpos.value = 0
      break
    case 'Enter':
      if (numpadInputStage == NumpadInputStage.start) {
        numberentry.value = numbersPrevious
        setFinalNumber(numbersPrevious)
      } else if (numpadInputStage == NumpadInputStage.entry) {
        numberentry.value = calcNumber()
        setFinalNumber(numberentry.value)
      }
      break
    case 'Backspace':
      numpadInputStage = NumpadInputStage.entry
      if (numbersClicked.at(-1) == '.') {
        numbersClicked.pop()
      }
      if (numbersClicked.length <= 1) {
        cursorpos.value = 0
      }
      numbersClicked.pop()
      if (numbersClicked.length === 0) {
        // Reset threading fields to null when fully erased
        switch (entryActive.value) {
          case ThreadingEntryType.threadPitch:
            clearThreadingParameter(ThreadingEntryType.threadPitch)
            break
          case ThreadingEntryType.threadXDepth:
            clearThreadingParameter(ThreadingEntryType.threadXDepth)
            break
          case ThreadingEntryType.threadZDepth:
            clearThreadingParameter(ThreadingEntryType.threadZDepth)
            break
          case ThreadingEntryType.threadAngle:
            clearThreadingParameter(ThreadingEntryType.threadAngle)
            break
          case ThreadingEntryType.threadZEnd:
            clearThreadingParameter(ThreadingEntryType.threadZEnd)
            break
          case ThreadingEntryType.threadXPullout:
            clearThreadingParameter(ThreadingEntryType.threadXPullout)
            break
          case ThreadingEntryType.threadZPullout:
            clearThreadingParameter(ThreadingEntryType.threadZPullout)
            break
          case ThreadingEntryType.threadFirstCut:
            clearThreadingParameter(ThreadingEntryType.threadFirstCut)
            break
          case ThreadingEntryType.threadCutMult:
            clearThreadingParameter(ThreadingEntryType.threadCutMult)
            break
          case ThreadingEntryType.threadMinCut:
            clearThreadingParameter(ThreadingEntryType.threadMinCut)
            break
          case ThreadingEntryType.threadSpringCuts:
            clearThreadingParameter(ThreadingEntryType.threadSpringCuts)
            break
          case TurningEntryType.turningTarget:
            clearTurningParameter(TurningEntryType.turningTarget)
            break
          case TurningEntryType.turningZEnd:
            clearTurningParameter(TurningEntryType.turningZEnd)
            break
          case TurningEntryType.turningFeedRate:
            clearTurningParameter(TurningEntryType.turningFeedRate)
            break
          case TurningEntryType.turningStepDown:
            clearTurningParameter(TurningEntryType.turningStepDown)
            break
          case TurningEntryType.turningSpringPasses:
            clearTurningParameter(TurningEntryType.turningSpringPasses)
            break
          case TurningEntryType.turningFinalStepDown:
            clearTurningParameter(TurningEntryType.turningFinalStepDown)
            break
          case TurningEntryType.turningTaperAngle:
            clearTurningParameter(TurningEntryType.turningTaperAngle)
            break
        }
        numpadInputStage = NumpadInputStage.none
        entryActive.value = 0
        numberentry.value = 0
      } else {
        numberentry.value = calcNumber()
      }
      break
    case 'PlusMinus':
      if (numpadInputStage == NumpadInputStage.start) {
        numbersNegative = !numbersNegative
        numberentry.value = numbersPrevious * (numbersNegative ? -1 : +1)
        setFinalNumber(numberentry.value)
      } else {
        numbersNegative = !numbersNegative
        numberentry.value = calcNumber()
      }
      break
    case 'Third':
      if (numpadInputStage == NumpadInputStage.start) {
        numberentry.value = numberentry.value / 3
        setFinalNumber(numberentry.value)
      }
      break
    case 'Half':
      if (numpadInputStage == NumpadInputStage.start) {
        numberentry.value = numberentry.value / 2
        setFinalNumber(numberentry.value)
      }
      break
    default:
      numpadInputStage = NumpadInputStage.entry
      numbersClicked.push(key)
      numberentry.value = calcNumber()
      break
  }
}

const zeroClicked = (entry: number) => {
  treatOffClickAsEnter()
  entryActive.value = 0
  switch (entry) {
    case EntryType.xPosition:
      setAxisValue('x', 0)
      scheduleHALOut()
      break
    case EntryType.zPosition:
      setAxisValue('z', 0)
      scheduleHALOut()
      break
    case EntryType.aPosition:
      setAxisValue('a', 0)
      scheduleHALOut()
      break
  }
}

const metricClicked = () => {
  treatOffClickAsEnter()
  metric.value = !metric.value
}

const otherClicked = () => {
  treatOffClickAsEnter()
  entryActive.value = 0
}

export function useNumpadEntry() {
  return {
    entryActive,
    numberentry,
    cursorpos,
    treatOffClickAsEnter,
    numberClicked,
    numPadClicked,
    zeroClicked,
    metricClicked,
    otherClicked
  }
}
