<template>
  <div class="m-2">
    <div class="flex flex-row">
      <DRODisplay
        class="mr-2 h-min"
        :entry-active="entryActive"
        :xpos="displayXPos"
        :zpos="displayZPos"
        :apos="apos"
        :rpms="rpmsSmoothed"
        :xpitch="xpitch"
        :zpitch="zpitch"
        :xlock="xpitchactive"
        :zlock="zpitchactive"
        :xpitchactive="xstepperactive"
        :zpitchactive="zstepperactive"
        :numberentry="numberentry"
        :xpitchlabel="xpitchlabel"
        :zpitchlabel="zpitchlabel"
        :metric="metric"
        :cursorpos="cursorpos"
        :diameter-mode="diameterMode"
        :show-x-pitch="true"
        :show-z-pitch="true"
        :tool-index="currentToolIndex"
        @number-clicked="numberClicked"
        @zero-clicked="zeroClicked"
        @pitch-clicked="pitchClicked"
        @metric-clicked="metricClicked"
        @other-clicked="otherClicked"
        @tool-clicked="openToolTable"
      />
      <div class="divider-vertical"></div>
      <Numpad class="" @num-pad-clicked="numPadClicked" />
    </div>
    <div class="divider-horizontal"></div>
    <div class="flex flex-row">
      <div class="grid dro-font-mode grid-nogutter p-3 pr-4 m-0 mt-2" style="width: 16em">
        <div class="col-12 align-content-center">Feed</div>
        <button
          size="large"
          class="col-12 dro-font-mode button-mode p-3 m-1"
          @click="feedModeLongitudinalClicked"
        >
          <span class="flex flex-row align-items-center">
            <i
              v-if="selectedFeedMode == FeedMode.longitudinal"
              class="pi pi-circle-fill mr-3"
              style="color: #ff0000"
            />
            <i v-else class="pi pi-circle mr-3" />
            ⬌ Longitudinal
          </span>
        </button>
        <button
          size="large"
          class="col-12 dro-font-mode button-mode p-3 m-1"
          @click="feedModeCrossClicked"
        >
          <span class="flex flex-row align-items-center">
            <i
              v-if="selectedFeedMode == FeedMode.cross"
              class="pi pi-circle-fill mr-3"
              style="color: #ff0000"
            />
            <i v-else class="pi pi-circle mr-3" />
            ⬍ Cross
          </span>
        </button>
        <button
          size="large"
          class="col-12 dro-font-mode button-mode p-3 m-1"
          @click="feedModeFrontCompoundClicked"
        >
          <span class="flex flex-row align-items-center">
            <i
              v-if="selectedFeedMode == FeedMode.frontCompound"
              class="pi pi-circle-fill mr-3"
              style="color: #ff0000"
            />
            <i v-else class="pi pi-circle mr-3" />
            ⬋ Front Compound
          </span>
        </button>
        <button
          size="large"
          class="col-12 dro-font-mode button-mode p-3 m-1"
          @click="feedModeBackCompoundClicked"
        >
          <span class="flex flex-row align-items-center">
            <i
              v-if="selectedFeedMode == FeedMode.backCompound"
              class="pi pi-circle-fill mr-3"
              style="color: #ff0000"
            />
            <i v-else class="pi pi-circle mr-3" />
            ⬉ Back Compound
          </span>
        </button>
      </div>
      <div class="grid dro-font-mode grid-nogutter p-3 pr-4 m-0 mt-2" style="width: 15em">
        <div class="col-12 align-content-center">Direction</div>
        <button
          size="large"
          class="col-12 dro-font-mode button-mode p-3 m-1"
          @click="directionModeForwardClicked"
        >
          <span class="flex flex-row align-items-center">
            <i
              v-if="selectedDirectionMode == DirectionMode.forward"
              class="pi pi-circle-fill mr-3"
              style="color: #ff0000"
            />
            <i v-else class="pi pi-circle mr-3" />
            {{ forwardIcon }} Forward
          </span>
        </button>
        <button
          size="large"
          class="col-12 dro-font-mode button-mode p-3 m-1"
          @click="directionModeReverseClicked"
        >
          <span class="flex flex-row align-items-center">
            <i
              v-if="selectedDirectionMode == DirectionMode.reverse"
              class="pi pi-circle-fill mr-3"
              style="color: #ff0000"
            />
            <i v-else class="pi pi-circle mr-3" />
            {{ reverseIcon }} Reverse
          </span>
        </button>
        <button
          size="large"
          class="col-12 dro-font-mode button-mode p-3 m-1"
          @click="directionModeHoldClicked"
        >
          <span class="flex flex-row align-items-center">
            <i
              v-if="selectedDirectionMode == DirectionMode.hold"
              class="pi pi-circle-fill mr-3"
              style="color: #ff0000"
            />
            <i v-else class="pi pi-circle mr-3" />
            ⏸ Hold
          </span>
        </button>
        <button
          size="large"
          class="col-12 dro-font-mode button-mode p-3 m-1"
          @click="directionModeIdleClicked"
        >
          <span class="flex flex-row align-items-center">
            <i
              v-if="selectedDirectionMode == DirectionMode.idle"
              class="pi pi-circle-fill mr-3"
              style="color: #ff0000"
            />
            <i v-else class="pi pi-circle mr-3" />
            ⏹ Idle
          </span>
        </button>
      </div>
      <div class="grid grid-nogutter mt-2 p-1 dro-font-mode" style="width: 24em">
        <div class="col-4 p-1"></div>
        <div class="col-4 p-1">
          <button
            class="button-arrow button-direction w-full h-full"
            @touchstart="touchStartUp"
            @touchend="touchEndUp"
            @touchcancel="touchEndUp"
            @touchleave="touchEndUp"
          >
            ⏶
          </button>
        </div>
        <div class="col-4 p-1"></div>
        <div class="col-4 p-1">
          <button
            class="button-arrow button-direction w-full h-full"
            @touchstart="touchStartLeft"
            @touchend="touchEndLeft"
            @touchcancel="touchEndLeft"
            @touchleave="touchEndLeft"
          >
            ⏴
          </button>
        </div>
        <div class="col-4 p-1">
          <button
            class="button-arrow button-direction w-full h-full"
            @touchstart="touchStop"
            @touchend="touchStop"
            @touchcancel="touchStop"
            @touchleave="touchStop"
          >
            STOP
          </button>
        </div>
        <div class="col-4 p-1">
          <button
            class="button-arrow button-direction w-full h-full"
            @touchstart="touchStartRight"
            @touchend="touchEndRight"
            @touchcancel="touchEndRight"
            @touchleave="touchEndRight"
          >
            ⏵
          </button>
        </div>
        <div class="col-4 p-1"></div>
        <div class="col-4 p-1">
          <button
            class="button-arrow button-direction w-full h-full"
            @touchstart="touchStartDown"
            @touchend="touchEndDown"
            @touchcancel="touchEndDown"
            @touchleave="touchEndDown"
          >
            ⏷
          </button>
        </div>
        <div class="col-4 p-1"></div>
      </div>
    </div>
    <DynamicDialog />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import DRODisplay from './DRODisplay.vue'
import Numpad from './Numpad.vue'
import { useHAL, FeedMode, DirectionMode } from '../composables/useHAL'
import { useSettings } from '../composables/useSettings'
import { useNumpadEntry } from '../composables/useNumpadEntry'
import { usePitchDialog } from '../composables/usePitchDialog'
import { useToolTable } from '../composables/useToolTable'

const {
  displayXPos,
  displayZPos,
  apos,
  rpmsSmoothed,
  xpitch,
  zpitch,
  xpitchactive,
  zpitchactive,
  xstepperactive,
  zstepperactive,
  xpitchlabel,
  zpitchlabel,
  selectedFeedMode,
  selectedDirectionMode,
  setButtonTime,
  scheduleButtonUp,
  stopJogNow
} = useHAL()
const { metric, diameterMode, currentToolIndex } = useSettings()
const {
  entryActive,
  numberentry,
  cursorpos,
  numberClicked,
  numPadClicked,
  zeroClicked,
  metricClicked,
  otherClicked
} = useNumpadEntry()
const { pitchClicked } = usePitchDialog()
const { openToolTable } = useToolTable()

const forwardIcon = computed(() => {
  switch (selectedFeedMode.value) {
    case FeedMode.longitudinal:
      return '⬅'
    case FeedMode.cross:
      return '⬆'
    case FeedMode.frontCompound:
      return '⬋'
    case FeedMode.backCompound:
      return '⬉'
    default:
      return ''
  }
})

const reverseIcon = computed(() => {
  switch (selectedFeedMode.value) {
    case FeedMode.longitudinal:
      return '⮕'
    case FeedMode.cross:
      return '⬇'
    case FeedMode.frontCompound:
      return '⬈'
    case FeedMode.backCompound:
      return '⬊'
    default:
      return ''
  }
})

const feedModeLongitudinalClicked = () => {
  selectedFeedMode.value = FeedMode.longitudinal
}
const feedModeCrossClicked = () => {
  selectedFeedMode.value = FeedMode.cross
}
const feedModeFrontCompoundClicked = () => {
  selectedFeedMode.value = FeedMode.frontCompound
}
const feedModeBackCompoundClicked = () => {
  selectedFeedMode.value = FeedMode.backCompound
}
const directionModeForwardClicked = () => {
  selectedDirectionMode.value = DirectionMode.forward
}
const directionModeReverseClicked = () => {
  selectedDirectionMode.value = DirectionMode.reverse
}
const directionModeHoldClicked = () => {
  selectedDirectionMode.value = DirectionMode.hold
}
const directionModeIdleClicked = () => {
  selectedDirectionMode.value = DirectionMode.idle
}

const touchStartUp = () => {
  setButtonTime('up', Date.now() / 1000)
}

const touchEndUp = () => {
  scheduleButtonUp()
  stopJogNow()
}

const touchStartLeft = () => {
  setButtonTime('left', Date.now() / 1000)
}

const touchEndLeft = () => {
  scheduleButtonUp()
  stopJogNow()
}

const touchStartRight = () => {
  setButtonTime('right', Date.now() / 1000)
}

const touchEndRight = () => {
  stopJogNow()
  scheduleButtonUp()
}

const touchStartDown = () => {
  setButtonTime('down', Date.now() / 1000)
}

const touchEndDown = () => {
  scheduleButtonUp()
  stopJogNow()
}

const touchStop = () => {
  scheduleButtonUp()
  stopJogNow()
}
</script>
