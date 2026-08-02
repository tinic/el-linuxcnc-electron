<template>
  <div class="flex-grow-1 flex flex-column p-2">
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
        :show-x-pitch="false"
        :show-z-pitch="false"
        :tool-index="currentToolIndex"
        @number-clicked="numberClicked"
        @zero-clicked="zeroClicked"
        @pitch-clicked="pitchClicked"
        @metric-clicked="threadMetricClicked"
        @other-clicked="otherClicked"
        @tool-clicked="openToolTable"
      />
      <div class="divider-vertical"></div>
      <Numpad class="" @num-pad-clicked="numPadClicked" />
    </div>
    <div class="divider-horizontal"></div>
    <div class="flex flex-row flex-grow-1">
      <div class="grid dro-font-mode grid-nogutter p-3 pr-4 m-0" style="width: 18em">
        <div class="col-12 align-content-center">Canned Cycles</div>
        <button
          size="large"
          class="col-12 dro-font-mode button-mode p-3 m-1"
          @click="cannedCycleClicked(CannedCycle.turning)"
        >
          <span class="flex flex-row align-items-center">
            <i
              v-if="selectedCannedCycle == CannedCycle.turning"
              class="pi pi-circle-fill mr-3"
              style="color: #ff0000"
            />
            <i v-else class="pi pi-circle mr-3" />
            Turning
          </span>
        </button>
        <button
          size="large"
          class="col-12 dro-font-mode button-mode p-3 m-1"
          @click="cannedCycleClicked(CannedCycle.threading)"
        >
          <span class="flex flex-row align-items-center">
            <i
              v-if="selectedCannedCycle == CannedCycle.threading"
              class="pi pi-circle-fill mr-3"
              style="color: #ff0000"
            />
            <i v-else class="pi pi-circle mr-3" />
            Threading
          </span>
        </button>
        <button
          size="large"
          class="col-12 dro-font-mode button-mode p-3 m-1"
          @click="cannedCycleClicked(CannedCycle.placeholder3)"
        >
          <span class="flex flex-row align-items-center">
            <i
              v-if="selectedCannedCycle == CannedCycle.placeholder3"
              class="pi pi-circle-fill mr-3"
              style="color: #ff0000"
            />
            <i v-else class="pi pi-circle mr-3" />
            Placeholder 3
          </span>
        </button>
        <button
          size="large"
          class="col-12 dro-font-mode button-mode p-3 m-1"
          @click="cannedCycleClicked(CannedCycle.placeholder4)"
        >
          <span class="flex flex-row align-items-center">
            <i
              v-if="selectedCannedCycle == CannedCycle.placeholder4"
              class="pi pi-circle-fill mr-3"
              style="color: #ff0000"
            />
            <i v-else class="pi pi-circle mr-3" />
            Placeholder 4
          </span>
        </button>
      </div>
      <div v-if="selectedCannedCycle == CannedCycle.threading" class="divider-vertical"></div>
      <div
        v-if="selectedCannedCycle == CannedCycle.threading"
        class="flex flex-column dro-font-mode p-1"
        style="width: 43em"
      >
        <div class="grid grid-nogutter flex-none">
          <div class="col-12 align-content-center mb-2">
            Threading{{ threadPresetName ? ` - ${threadPresetName}` : '' }}
          </div>
          <!-- Note: X Start uses current position, Z Start is always 0 -->

          <!-- Row 1 -->
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showLabelPopover($event, 'P')"
          >
            Pitch
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != ThreadingEntryType.threadPitch && threadPitch === null
                }
              ]"
              :style="{
                backgroundColor: entryActive == ThreadingEntryType.threadPitch ? '#666' : '#333'
              }"
              :title="
                entryActive == ThreadingEntryType.threadPitch
                  ? String(numberentry)
                  : String(threadPitch ?? 'Pitch')
              "
              @click="numberClicked(ThreadingEntryType.threadPitch, threadPitch || 0)"
            >
              {{
                entryActive == ThreadingEntryType.threadPitch
                  ? numberentry
                  : threadPitch ?? 'Pitch'
              }}
            </button>
          </div>
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showLabelPopover($event, 'D')"
          >
            Ø
          </div>
          <div class="col-4 p-1">
            <div
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                { 'placeholder-text': threadDiameter === null }
              ]"
              :style="{ backgroundColor: '#333', color: '#999', cursor: 'default' }"
              :title="threadDiameter ?? 'Major Ø / Drill Size'"
            >
              {{ threadDiameter ?? 'Major Ø / Drill' }}
            </div>
          </div>
          <div class="col-2 p-0"></div>

          <!-- Row 2 -->
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showLabelPopover($event, 'ZD')"
          >
            ZDepth
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != ThreadingEntryType.threadZDepth && threadZDepth === null
                }
              ]"
              :style="{
                backgroundColor: entryActive == ThreadingEntryType.threadZDepth ? '#666' : '#333'
              }"
              :title="
                entryActive == ThreadingEntryType.threadZDepth
                  ? String(numberentry)
                  : String(threadZDepth ?? 'Z Depth')
              "
              @click="numberClicked(ThreadingEntryType.threadZDepth, threadZDepth || 0)"
            >
              {{
                entryActive == ThreadingEntryType.threadZDepth
                  ? numberentry
                  : threadZDepth ?? 'Z Depth'
              }}
            </button>
          </div>
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showLabelPopover($event, 'XD')"
          >
            XDepth
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != ThreadingEntryType.threadXDepth && threadXDepth === null
                }
              ]"
              :style="{
                backgroundColor: entryActive == ThreadingEntryType.threadXDepth ? '#666' : '#333'
              }"
              :title="
                entryActive == ThreadingEntryType.threadXDepth
                  ? String(numberentry)
                  : String(threadXDepth ?? 'X Depth')
              "
              @click="numberClicked(ThreadingEntryType.threadXDepth, threadXDepth || 0)"
            >
              {{
                entryActive == ThreadingEntryType.threadXDepth
                  ? numberentry
                  : threadXDepth ?? 'X Depth'
              }}
            </button>
          </div>
          <div class="col-2 p-0"></div>

          <!-- Row 3 -->
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showLabelPopover($event, 'ZE')"
          >
            ZEnd
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != ThreadingEntryType.threadZEnd && threadZEnd === null
                }
              ]"
              :style="{
                backgroundColor: entryActive == ThreadingEntryType.threadZEnd ? '#666' : '#333'
              }"
              :title="
                entryActive == ThreadingEntryType.threadZEnd
                  ? String(numberentry)
                  : String(threadZEnd ?? 'Z End')
              "
              @click="numberClicked(ThreadingEntryType.threadZEnd, threadZEnd || 0)"
            >
              {{
                entryActive == ThreadingEntryType.threadZEnd ? numberentry : threadZEnd ?? 'Z End'
              }}
            </button>
          </div>
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showLabelPopover($event, 'A')"
          >
            Angle
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != ThreadingEntryType.threadAngle && threadAngle === null
                }
              ]"
              :style="{
                backgroundColor: entryActive == ThreadingEntryType.threadAngle ? '#666' : '#333'
              }"
              :title="
                entryActive == ThreadingEntryType.threadAngle
                  ? String(numberentry)
                  : String(threadAngle ?? 'Angle')
              "
              @click="numberClicked(ThreadingEntryType.threadAngle, threadAngle || 0)"
            >
              {{
                entryActive == ThreadingEntryType.threadAngle
                  ? numberentry
                  : threadAngle ?? 'Angle'
              }}
            </button>
          </div>
          <div class="col-2 p-0"></div>

          <!-- Row 4 -->
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showLabelPopover($event, 'ZP')"
          >
            ZPull
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != ThreadingEntryType.threadZPullout && threadZPullout === null
                }
              ]"
              :style="{
                backgroundColor:
                  entryActive == ThreadingEntryType.threadZPullout ? '#666' : '#333'
              }"
              :title="
                entryActive == ThreadingEntryType.threadZPullout
                  ? String(numberentry)
                  : String(threadZPullout ?? 'Z Pullout')
              "
              @click="numberClicked(ThreadingEntryType.threadZPullout, threadZPullout || 0)"
            >
              {{
                entryActive == ThreadingEntryType.threadZPullout
                  ? numberentry
                  : threadZPullout ?? 'Z Pullout'
              }}
            </button>
          </div>
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showLabelPopover($event, 'XP')"
          >
            XPull
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != ThreadingEntryType.threadXPullout && threadXPullout === null
                }
              ]"
              :style="{
                backgroundColor:
                  entryActive == ThreadingEntryType.threadXPullout ? '#666' : '#333'
              }"
              :title="
                entryActive == ThreadingEntryType.threadXPullout
                  ? String(numberentry)
                  : String(threadXPullout ?? 'X Pullout')
              "
              @click="numberClicked(ThreadingEntryType.threadXPullout, threadXPullout || 0)"
            >
              {{
                entryActive == ThreadingEntryType.threadXPullout
                  ? numberentry
                  : threadXPullout ?? 'X Pullout'
              }}
            </button>
          </div>
          <div class="col-2 p-0"></div>

          <!-- Row 5 -->
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showLabelPopover($event, 'CM')"
          >
            Mult
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != ThreadingEntryType.threadCutMult && threadCutMult === null
                }
              ]"
              :style="{
                backgroundColor: entryActive == ThreadingEntryType.threadCutMult ? '#666' : '#333'
              }"
              :title="
                entryActive == ThreadingEntryType.threadCutMult
                  ? String(numberentry)
                  : String(threadCutMult ?? 'Cut Multiplier')
              "
              @click="numberClicked(ThreadingEntryType.threadCutMult, threadCutMult || 0)"
            >
              {{
                entryActive == ThreadingEntryType.threadCutMult
                  ? numberentry
                  : threadCutMult ?? 'Cut Multiplier'
              }}
            </button>
          </div>
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showLabelPopover($event, 'FC')"
          >
            1stCut
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != ThreadingEntryType.threadFirstCut && threadFirstCut === null
                }
              ]"
              :style="{
                backgroundColor:
                  entryActive == ThreadingEntryType.threadFirstCut ? '#666' : '#333'
              }"
              :title="
                entryActive == ThreadingEntryType.threadFirstCut
                  ? String(numberentry)
                  : String(threadFirstCut ?? 'First Cut')
              "
              @click="numberClicked(ThreadingEntryType.threadFirstCut, threadFirstCut || 0)"
            >
              {{
                entryActive == ThreadingEntryType.threadFirstCut
                  ? numberentry
                  : threadFirstCut ?? 'First Cut'
              }}
            </button>
          </div>
          <div class="col-2 p-0"></div>

          <!-- Row 6 -->
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showLabelPopover($event, 'SC')"
          >
            Spring
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != ThreadingEntryType.threadSpringCuts &&
                    threadSpringCuts === null
                }
              ]"
              :style="{
                backgroundColor:
                  entryActive == ThreadingEntryType.threadSpringCuts ? '#666' : '#333'
              }"
              :title="
                entryActive == ThreadingEntryType.threadSpringCuts
                  ? String(numberentry)
                  : String(threadSpringCuts ?? 'Spring Cuts')
              "
              @click="numberClicked(ThreadingEntryType.threadSpringCuts, threadSpringCuts || 0)"
            >
              {{
                entryActive == ThreadingEntryType.threadSpringCuts
                  ? numberentry
                  : threadSpringCuts ?? 'Spring Cuts'
              }}
            </button>
          </div>
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showLabelPopover($event, 'MC')"
          >
            MinCut
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != ThreadingEntryType.threadMinCut && threadMinCut === null
                }
              ]"
              :style="{
                backgroundColor: entryActive == ThreadingEntryType.threadMinCut ? '#666' : '#333'
              }"
              :title="
                entryActive == ThreadingEntryType.threadMinCut
                  ? String(numberentry)
                  : String(threadMinCut ?? 'Min Cut')
              "
              @click="numberClicked(ThreadingEntryType.threadMinCut, threadMinCut || 0)"
            >
              {{
                entryActive == ThreadingEntryType.threadMinCut
                  ? numberentry
                  : threadMinCut ?? 'Min Cut'
              }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
        </div>

        <!-- Spacer to push buttons to bottom -->
        <div class="flex-grow-1"></div>

        <!-- Start, Stop, Preset and Reset buttons centered as a group -->
        <div class="flex justify-content-center gap-2 p-1">
          <button
            class="dro-font-mode button-mode p-2"
            style="background: #555; color: #ffffff; width: 6em"
            @click="openThreadPresetDialog(metric, updatePitchFromThread)"
          >
            ...
          </button>
          <button
            class="dro-font-mode button-mode p-2"
            style="background: #555; color: #ffffff; width: 6em"
            @click="threadResetClicked"
          >
            Reset
          </button>
          <button
            class="dro-font-mode button-mode p-2"
            style="background: #22c55e; color: #ffffff; width: 6em"
            @click="threadStartClicked"
          >
            ⏵ Start
          </button>
          <button
            class="dro-font-mode button-mode p-2"
            style="background: #ef4444; color: #ffffff; width: 6em"
            @click="threadStopClicked"
          >
            ⏹ Stop
          </button>
        </div>

        <!-- Threading Label Popovers -->
        <Popover :ref="(el) => (threadingPopovers['P'] = el)" class="threading-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ threadingDescriptions['P'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (threadingPopovers['D'] = el)" class="threading-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ threadingDescriptions['D'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (threadingPopovers['XD'] = el)" class="threading-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ threadingDescriptions['XD'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (threadingPopovers['ZD'] = el)" class="threading-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ threadingDescriptions['ZD'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (threadingPopovers['A'] = el)" class="threading-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ threadingDescriptions['A'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (threadingPopovers['ZE'] = el)" class="threading-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ threadingDescriptions['ZE'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (threadingPopovers['XP'] = el)" class="threading-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ threadingDescriptions['XP'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (threadingPopovers['ZP'] = el)" class="threading-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ threadingDescriptions['ZP'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (threadingPopovers['FC'] = el)" class="threading-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ threadingDescriptions['FC'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (threadingPopovers['CM'] = el)" class="threading-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ threadingDescriptions['CM'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (threadingPopovers['MC'] = el)" class="threading-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ threadingDescriptions['MC'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (threadingPopovers['SC'] = el)" class="threading-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ threadingDescriptions['SC'] }}
          </div>
        </Popover>
      </div>

      <!-- Turning Cycle Panel -->
      <div v-if="selectedCannedCycle == CannedCycle.turning" class="divider-vertical"></div>
      <div
        v-if="selectedCannedCycle == CannedCycle.turning"
        class="flex flex-column dro-font-mode p-1"
        style="width: 43em"
      >
        <div class="grid grid-nogutter flex-none">
          <div class="col-12 align-content-center mb-2">
            Turning{{ turningPresetName ? ` - ${turningPresetName}` : '' }}
          </div>

          <!-- Row 1: X Start, X End -->
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showTurningLabelPopover($event, 'XS')"
          >
            Target
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != TurningEntryType.turningTarget && turningTarget === null
                }
              ]"
              :style="{
                backgroundColor: entryActive == TurningEntryType.turningTarget ? '#666' : '#333'
              }"
              :title="
                entryActive == TurningEntryType.turningTarget
                  ? String(numberentry)
                  : String(displayTurningTarget ?? 'Target')
              "
              @click="numberClicked(TurningEntryType.turningTarget, displayTurningTarget || 0)"
            >
              {{
                entryActive == TurningEntryType.turningTarget
                  ? numberentry
                  : displayTurningTarget ?? 'Target'
              }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          <div class="col-1 p-0"></div>
          <div class="col-4 p-0"></div>

          <!-- Row 2: Z End -->
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showTurningLabelPopover($event, 'ZE')"
          >
            Length
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != TurningEntryType.turningZEnd && turningZEnd === null
                }
              ]"
              :style="{
                backgroundColor: entryActive == TurningEntryType.turningZEnd ? '#666' : '#333'
              }"
              :title="
                entryActive == TurningEntryType.turningZEnd
                  ? String(numberentry)
                  : String(turningZEnd ?? 'Z End')
              "
              @click="numberClicked(TurningEntryType.turningZEnd, turningZEnd || 0)"
            >
              {{
                entryActive == TurningEntryType.turningZEnd ? numberentry : turningZEnd ?? 'Z End'
              }}
            </button>
          </div>
          <div class="col-7 p-0"></div>

          <!-- Row 3: Feed Rate, Step Down -->
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showTurningLabelPopover($event, 'F')"
          >
            Feed
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != TurningEntryType.turningFeedRate && turningFeedRate === null
                }
              ]"
              :style="{
                backgroundColor: entryActive == TurningEntryType.turningFeedRate ? '#666' : '#333'
              }"
              :title="
                entryActive == TurningEntryType.turningFeedRate
                  ? String(numberentry)
                  : String(turningFeedRate ?? 'Feed Rate')
              "
              @click="numberClicked(TurningEntryType.turningFeedRate, turningFeedRate || 0)"
            >
              {{
                entryActive == TurningEntryType.turningFeedRate
                  ? numberentry
                  : turningFeedRate ?? 'Feed Rate'
              }}
            </button>
          </div>
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showTurningLabelPopover($event, 'SD')"
          >
            Step
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != TurningEntryType.turningStepDown && turningStepDown === null
                }
              ]"
              :style="{
                backgroundColor: entryActive == TurningEntryType.turningStepDown ? '#666' : '#333'
              }"
              :title="
                entryActive == TurningEntryType.turningStepDown
                  ? String(numberentry)
                  : String(turningStepDown ?? 'Step Down')
              "
              @click="numberClicked(TurningEntryType.turningStepDown, turningStepDown || 0)"
            >
              {{
                entryActive == TurningEntryType.turningStepDown
                  ? numberentry
                  : turningStepDown ?? 'Step Down'
              }}
            </button>
          </div>
          <div class="col-2 p-0"></div>

          <!-- Row 4: Spring Passes, Finishing Allowance -->
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showTurningLabelPopover($event, 'SP')"
          >
            Spring
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != TurningEntryType.turningSpringPasses &&
                    turningSpringPasses === null
                }
              ]"
              :style="{
                backgroundColor:
                  entryActive == TurningEntryType.turningSpringPasses ? '#666' : '#333'
              }"
              :title="
                entryActive == TurningEntryType.turningSpringPasses
                  ? String(numberentry)
                  : String(turningSpringPasses ?? 'Spring Passes')
              "
              @click="
                numberClicked(TurningEntryType.turningSpringPasses, turningSpringPasses || 0)
              "
            >
              {{
                entryActive == TurningEntryType.turningSpringPasses
                  ? numberentry
                  : turningSpringPasses ?? 'Spring Passes'
              }}
            </button>
          </div>
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showTurningLabelPopover($event, 'FS')"
          >
            Final
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != TurningEntryType.turningFinalStepDown &&
                    turningFinalStepDown === null
                }
              ]"
              :style="{
                backgroundColor:
                  entryActive == TurningEntryType.turningFinalStepDown ? '#666' : '#333'
              }"
              :title="
                entryActive == TurningEntryType.turningFinalStepDown
                  ? String(numberentry)
                  : String(turningFinalStepDown ?? 'Final Step Down')
              "
              @click="
                numberClicked(TurningEntryType.turningFinalStepDown, turningFinalStepDown || 0)
              "
            >
              {{
                entryActive == TurningEntryType.turningFinalStepDown
                  ? numberentry
                  : turningFinalStepDown ?? 'Final Step Down'
              }}
            </button>
          </div>
          <div class="col-2 p-0"></div>

          <!-- Row 5: Taper Angle -->
          <div
            class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer"
            @click="showTurningLabelPopover($event, 'TA')"
          >
            Taper
          </div>
          <div class="col-4 p-1">
            <button
              :class="[
                'w-full text-left dro-font-mode button-mode p-1 truncate',
                {
                  'placeholder-text':
                    entryActive != TurningEntryType.turningTaperAngle &&
                    turningTaperAngle === null
                }
              ]"
              :style="{
                backgroundColor:
                  entryActive == TurningEntryType.turningTaperAngle ? '#666' : '#333'
              }"
              :title="
                entryActive == TurningEntryType.turningTaperAngle
                  ? String(numberentry)
                  : String(turningTaperAngle ?? 'Taper Angle')
              "
              @click="numberClicked(TurningEntryType.turningTaperAngle, turningTaperAngle || 0)"
            >
              {{
                entryActive == TurningEntryType.turningTaperAngle
                  ? numberentry
                  : turningTaperAngle ?? 'Taper Angle'
              }}
            </button>
          </div>
          <div class="col-7 p-0"></div>
        </div>

        <!-- Spacer to push buttons to bottom -->
        <div class="flex-grow-1"></div>

        <!-- Buttons -->
        <div class="flex justify-content-center gap-2 p-1">
          <button
            class="dro-font-mode button-mode p-2"
            style="background: #555; color: #ffffff; width: 3em"
            @click="openTurningPresetDialog(metric, updatePitchFromTurning)"
          >
            ...
          </button>
          <button
            class="dro-font-mode button-mode p-2"
            style="background: #555; color: #ffffff; width: 6em"
            @click="turningResetClicked"
          >
            Reset
          </button>
          <button
            class="dro-font-mode button-mode p-2"
            style="background: #22c55e; color: #ffffff; width: 6em"
            @click="turningStartClicked"
          >
            ⏵ Start
          </button>
          <button
            class="dro-font-mode button-mode p-2"
            style="background: #ef4444; color: #ffffff; width: 6em"
            @click="turningStopClicked"
          >
            ⏹ Stop
          </button>
        </div>

        <!-- Turning Label Popovers -->
        <Popover :ref="(el) => (turningPopovers['XS'] = el)" class="turning-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ turningDescriptions['XS'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (turningPopovers['XE'] = el)" class="turning-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ turningDescriptions['XE'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (turningPopovers['ZS'] = el)" class="turning-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ turningDescriptions['ZS'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (turningPopovers['ZE'] = el)" class="turning-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ turningDescriptions['ZE'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (turningPopovers['F'] = el)" class="turning-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ turningDescriptions['F'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (turningPopovers['SD'] = el)" class="turning-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ turningDescriptions['SD'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (turningPopovers['SP'] = el)" class="turning-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ turningDescriptions['SP'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (turningPopovers['FS'] = el)" class="turning-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ turningDescriptions['FS'] }}
          </div>
        </Popover>
        <Popover :ref="(el) => (turningPopovers['TA'] = el)" class="turning-popover">
          <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line">
            {{ turningDescriptions['TA'] }}
          </div>
        </Popover>
      </div>
    </div>
    <DynamicDialog />

    <!-- Operation Preview Dialog -->
    <Dialog
      v-model:visible="showOperationPreview"
      modal
      :show-header="false"
      :draggable="false"
      :closable="false"
      :style="{ width: '90vw', height: '90vh' }"
      :pt="{ content: { style: 'padding: 0; height: 100%;' } }"
    >
      <OperationPreview
        v-if="currentOperation"
        :operation="currentOperation"
        @continue="onPreviewContinue"
        @cancel="onPreviewCancel"
      />
    </Dialog>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import Dialog from 'primevue/dialog'
import Popover from 'primevue/popover'
import DRODisplay from './DRODisplay.vue'
import Numpad from './Numpad.vue'
import OperationPreview from './OperationPreview.vue'
import { useHAL } from '../composables/useHAL'
import { useSettings } from '../composables/useSettings'
import {
  useCannedCycles,
  ThreadingEntryType,
  TurningEntryType
} from '../composables/useCannedCycles'
import { useNumpadEntry } from '../composables/useNumpadEntry'
import { usePitchDialog } from '../composables/usePitchDialog'
import { useToolTable } from '../composables/useToolTable'

const {
  displayXPos,
  displayZPos,
  apos,
  rpmsSmoothed,
  xpos,
  xpitch,
  zpitch,
  xpitchactive,
  zpitchactive,
  xstepperactive,
  zstepperactive,
  xpitchlabel,
  zpitchlabel,
  getHalIn,
  getAxisOffset,
  putLinuxCNC,
  putAbort,
  putThreading,
  generateThreadingGcode,
  putTurning,
  generateTurningGcode,
  cleanupCannedCycles
} = useHAL()
const { metric, diameterMode, currentToolIndex } = useSettings()
const {
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
  threadingPopovers,
  threadingDescriptions,
  showLabelPopover,
  showTurningLabelPopover,
  validateThreadingParameters,
  generateThreadingParams,
  openThreadPresetDialog,
  resetThreadingParameters,
  convertThreadingParameters,
  turningTarget,
  turningZEnd,
  turningFeedRate,
  turningStepDown,
  turningFinalStepDown,
  turningTaperAngle,
  turningSpringPasses,
  turningPresetName,
  turningPopovers,
  turningDescriptions,
  validateTurningParameters,
  generateTurningParams,
  openTurningPresetDialog,
  resetTurningParameters,
  convertTurningParameters,
  updatePitchFromThread,
  updatePitchFromTurning
} = useCannedCycles()
const {
  entryActive,
  numberentry,
  cursorpos,
  treatOffClickAsEnter,
  numberClicked,
  numPadClicked,
  zeroClicked,
  otherClicked
} = useNumpadEntry()
const { pitchClicked } = usePitchDialog()
const { openToolTable } = useToolTable()

enum CannedCycle {
  none = 0,
  threading = 1,
  turning = 2,
  placeholder3 = 3,
  placeholder4 = 4
}
const selectedCannedCycle = ref(CannedCycle.none)

const cannedCycleClicked = (cycle: CannedCycle) => {
  selectedCannedCycle.value = cycle
}

// Operation Preview state
const showOperationPreview = ref(false)
const showBackplot = ref(false)
const currentOperation = ref<any>(null)
const pendingOperationExecution = ref<(() => void) | null>(null)

// Generic operation preview system
const showOperationPreviewDialog = (operationData: any, executeCallback: () => void) => {
  currentOperation.value = operationData
  pendingOperationExecution.value = executeCallback
  showOperationPreview.value = true
}

const onPreviewContinue = () => {
  showOperationPreview.value = false
  if (pendingOperationExecution.value) {
    pendingOperationExecution.value()
    pendingOperationExecution.value = null
  }
  currentOperation.value = null
}

const onPreviewCancel = () => {
  showOperationPreview.value = false
  pendingOperationExecution.value = null
  currentOperation.value = null
}

// Computed display values for turning parameters in diameter mode
const displayTurningTarget = computed(() => {
  if (turningTarget.value === null) {
    return null
  }
  return diameterMode.value ? turningTarget.value * 2 : turningTarget.value
})

const threadStartClicked = async () => {
  treatOffClickAsEnter()
  entryActive.value = 0

  // Validate and execute threading using composable
  const isValid = validateThreadingParameters()
  if (!isValid) {
    return
  }

  // Get HAL positions and generate threading operation
  try {
    const halIn = await getHalIn()
    const currentXPos = halIn.position_x - getAxisOffset('x')
    const currentZPos = halIn.position_z - getAxisOffset('z')
    const currentAPos = halIn.position_a - getAxisOffset('a')
    const params = generateThreadingParams(currentXPos, currentZPos, currentAPos)

    // Generate G-code for preview
    const result = await generateThreadingGcode(params)
    if (result && result.gcode) {
      // Create G-code string for backplot
      const gcodeString = result.gcode.join('\n')
      const gcodeBase64 = btoa(gcodeString)

      // Send to backplot generator
      const backplotData = await putLinuxCNC('backplot', { gcode: gcodeBase64 })
      if (backplotData) {
        // Create operation data for preview
        const operationData = {
          name: 'Threading Operation',
          type: 'threading',
          parameters: {
            Pitch: threadPitch.value,
            XDepth: threadXDepth.value,
            ZDepth: threadZDepth.value,
            ZEnd: threadZEnd.value,
            Angle: threadAngle.value,
            XPullout: threadXPullout.value,
            ZPullout: threadZPullout.value,
            FirstCut: threadFirstCut.value,
            CutMult: threadCutMult.value,
            MinCut: threadMinCut.value,
            SpringCuts: threadSpringCuts.value
          },
          gcode: result.gcode,
          backplotData: backplotData
        }

        // Show preview dialog with callback to execute threading
        const putThreadingCallback = () => {
          putThreading(params)
        }

        showOperationPreviewDialog(operationData, putThreadingCallback)
      }
    }
  } catch (error) {
    console.error('Threading operation failed:', error)
    alert('Failed to start threading operation. Please check parameters and try again.')
  }
}

const threadStopClicked = () => {
  treatOffClickAsEnter()
  entryActive.value = 0

  // Abort current operation (gentler than emergency stop)
  putAbort()
  cleanupCannedCycles()
}

const threadResetClicked = () => {
  treatOffClickAsEnter()
  entryActive.value = 0

  // Hide backplot
  showBackplot.value = false

  // Reset all threading parameters using composable function
  resetThreadingParameters()
  updatePitchFromThread()
}

const threadMetricClicked = () => {
  treatOffClickAsEnter()
  metric.value = !metric.value

  // Always convert both threading and turning parameters
  convertThreadingParameters(metric.value)
  convertTurningParameters(metric.value)

  // Update displays based on active cycle
  if (selectedCannedCycle.value === CannedCycle.threading) {
    updatePitchFromThread()
  } else if (selectedCannedCycle.value === CannedCycle.turning) {
    updatePitchFromTurning()
  }
}

// Turning cycle handlers
const turningStartClicked = async () => {
  treatOffClickAsEnter()
  entryActive.value = 0

  // Validate and execute turning using composable
  const errors = validateTurningParameters()
  if (errors.length > 0) {
    console.log('Turning validation errors:', errors)
    return
  }

  // Get HAL positions and generate turning operation
  try {
    const halIn = await getHalIn()
    const currentXPos = halIn.position_x - getAxisOffset('x')
    const currentZPos = halIn.position_z - getAxisOffset('z')
    const currentAPos = halIn.position_a - getAxisOffset('a')
    const params = generateTurningParams(currentXPos, currentZPos, currentAPos)

    // Generate G-code for preview
    const result = await generateTurningGcode(params)
    if (result && result.gcode) {
      // Create G-code string for backplot
      const gcodeString = result.gcode.join('\n')
      const gcodeBase64 = btoa(gcodeString)

      // Send to backplot generator
      const backplotData = await putLinuxCNC('backplot', { gcode: gcodeBase64 })
      if (backplotData) {
        // Create operation data for preview
        const operationData = {
          name: 'Turning Operation',
          type: 'turning',
          parameters: {
            Target: turningTarget.value,
            Stock: xpos.value,
            ZEnd: turningZEnd.value,
            FeedRate: turningFeedRate.value,
            StepDown: turningStepDown.value,
            SpringPasses: turningSpringPasses.value,
            FinalStepDown: turningFinalStepDown.value,
            TaperAngle: turningTaperAngle.value
          },
          gcode: result.gcode,
          backplotData: backplotData
        }

        // Show preview dialog with callback to execute turning
        const putTurningCallback = () => {
          putTurning(params)
        }

        showOperationPreviewDialog(operationData, putTurningCallback)
      }
    }
  } catch (error) {
    console.error('Error starting turning cycle:', error)
  }
}

const turningStopClicked = () => {
  treatOffClickAsEnter()
  entryActive.value = 0

  // Abort current operation (gentler than emergency stop)
  putAbort()
  cleanupCannedCycles()
}

const turningResetClicked = () => {
  treatOffClickAsEnter()
  entryActive.value = 0

  // Hide backplot
  showBackplot.value = false

  // Reset all turning parameters using composable function
  resetTurningParameters()
  updatePitchFromTurning()
}
</script>
