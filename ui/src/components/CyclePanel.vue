<script setup lang="ts">
import { computed, onBeforeUnmount, ref, shallowRef, watch, type DeepReadonly } from 'vue'

import {
  applyThreadPreset,
  applyTurningPreset,
  cycles,
  resetThreading,
  resetTurning,
  setThreadSide,
  setThreading,
  setTurning,
  threadingFields,
  turningFields,
  type ThreadingKey,
  type TurningKey,
} from '../cycles'
import { activeField, settle } from '../entry'
import { clearPlan, panel, send, updatePreferences } from '../machine'
import { threadSections, turningSections, type ThreadPreset, type TurningPreset } from '../presets'
import type { LiveTool } from '../backplot'
import type { Backplot, PlotPoint } from '../protocol'
import BackplotCanvas from './BackplotCanvas.vue'
import CycleDiagram from './CycleDiagram.vue'
import ModalDialog from './ModalDialog.vue'
import ParamGrid from './ParamGrid.vue'
import PresetPicker from './PresetPicker.vue'

const choosingPreset = ref(false)
const waitingForPlan = ref(false)

const plan = computed(() => (panel.plan?.kind === cycles.selected ? panel.plan : null))
const issues = computed(() => plan.value?.issues ?? [])
const previewing = computed(
  () =>
    waitingForPlan.value &&
    plan.value !== null &&
    plan.value.issues.length === 0 &&
    plan.value.error === '' &&
    plan.value.backplot !== null,
)
const running = computed(() => panel.status?.task.program_running ?? false)

const title = computed(() => (cycles.selected === 'threading' ? 'Threading' : 'Turning'))

/**
 * The picked preset, shown exactly as written ("6g" and "6G" are different thread tolerances), with
 * a thread's major diameter / tap drill.
 */
const presetName = computed(() => {
  if (cycles.selected === 'threading') {
    const preset = cycles.threadingPreset
    return preset ? `${preset.name} · Ø ${preset.diameter}` : ''
  }
  return cycles.turningPreset ?? ''
})

function chooseSide(internal: boolean): void {
  settle()
  clearPlan()
  setThreadSide(internal)
}

/** Field whose label was tapped last (help), for the diagram. */
const inspected = ref<string | null>(null)

/** The diagram highlights the field being edited, else the one whose label was tapped. */
const highlight = computed(() => {
  const editing = activeField.value
  const prefix = `${cycles.selected}.`
  return editing?.startsWith(prefix) === true ? editing.slice(prefix.length) : inspected.value
})

function choose(kind: 'threading' | 'turning'): void {
  settle()
  cycles.selected = kind
  inspected.value = null
  clearPlan()
  waitingForPlan.value = false
}

function onThreading(key: ThreadingKey, value: number | null): void {
  setThreading(key, value)
}

function onTurning(key: TurningKey, value: number | null): void {
  setTurning(key, value)
}

function reset(): void {
  settle()
  clearPlan()
  if (cycles.selected === 'threading') {
    resetThreading()
  } else {
    resetTurning()
  }
}

function requestPlan(): void {
  settle()
  clearPlan()
  waitingForPlan.value = true
  if (cycles.selected === 'threading') {
    send({ type: 'plan_threading', form: { ...cycles.threading } })
  } else {
    send({ type: 'plan_turning', form: { ...cycles.turning } })
  }
}

// Live view: while a started cycle runs, the diagram gives way to its backplot with the tool's
// actual position and track. The trail is a plain array (thousands of points); `trailVersion`
// tells the canvas it changed.
interface LiveRun {
  plot: DeepReadonly<Backplot>
  start: PlotPoint
  seenRunning: boolean
}
const liveRun = shallowRef<LiveRun | null>(null)
let trail: PlotPoint[] = []
const trailVersion = ref(0)
let liveTimer = 0
const startGraceMs = 5000
const finishedHoldMs = 3000
const maxTrailPoints = 20000

function endLiveView(afterMs: number): void {
  window.clearTimeout(liveTimer)
  liveTimer = window.setTimeout(() => {
    liveRun.value = null
    trail = []
  }, afterMs)
}

watch(
  () => panel.status,
  (status) => {
    const run = liveRun.value
    if (!run || !status) {
      return
    }
    const position = { x: status.readout.x, z: status.readout.z }
    const last = trail.at(-1)
    if (!last || Math.hypot(last.x - position.x, last.z - position.z) > 0.002) {
      trail.push(position)
      if (trail.length > maxTrailPoints) {
        trail.splice(0, trail.length - maxTrailPoints)
      }
      trailVersion.value += 1
    }
    if (status.task.program_running) {
      if (!run.seenRunning) {
        run.seenRunning = true
        window.clearTimeout(liveTimer)
      }
    } else if (run.seenRunning) {
      run.seenRunning = false
      endLiveView(finishedHoldMs)
    }
  },
)

const liveTool = computed<LiveTool | null>(() => {
  const status = panel.status
  if (!liveRun.value || !status) {
    return null
  }
  return { position: { x: status.readout.x, z: status.readout.z }, trail, revision: trailVersion.value }
})

onBeforeUnmount(() => {
  window.clearTimeout(liveTimer)
})

function runCycle(): void {
  const current = plan.value
  if (current?.backplot) {
    trail = []
    liveRun.value = { plot: current.backplot, start: { ...current.start }, seenRunning: false }
    // A cycle that does not start (refused, with an error notice) gives the diagram back.
    endLiveView(startGraceMs)
  }
  waitingForPlan.value = false
  send({ type: 'start_cycle' })
  clearPlan()
}

function cancelPreview(): void {
  waitingForPlan.value = false
  clearPlan()
}

function stop(): void {
  settle()
  send({ type: 'abort_cycle' })
}

function pickThread(preset: ThreadPreset): void {
  applyThreadPreset(preset)
  choosingPreset.value = false
}

function pickTurning(preset: TurningPreset): void {
  applyTurningPreset(preset)
  choosingPreset.value = false
}

const previewRows = computed(() => {
  const gcode = plan.value?.gcode ?? []
  return gcode.filter((line) => !line.startsWith('('))
})
</script>

<template>
  <div class="cycles">
    <section class="form">
      <header class="form-head">
        <nav class="kinds">
          <button :class="{ active: cycles.selected === 'turning' }" @click="choose('turning')">Turning</button>
          <button :class="{ active: cycles.selected === 'threading' }" @click="choose('threading')">
            Threading
          </button>
        </nav>
        <nav v-if="cycles.selected === 'threading'" class="kinds side">
          <button :class="{ active: !cycles.threading.internal }" @click="chooseSide(false)">External</button>
          <button :class="{ active: cycles.threading.internal }" @click="chooseSide(true)">Internal</button>
        </nav>
        <span v-if="presetName" class="preset-name">{{ presetName }}</span>
      </header>
      <ParamGrid
        v-if="cycles.selected === 'threading'"
        prefix="threading"
        :fields="threadingFields"
        :values="cycles.threading"
        :issues="issues"
        @change="onThreading"
        @inspect="inspected = $event"
      />
      <ParamGrid
        v-else
        prefix="turning"
        :fields="turningFields"
        :values="cycles.turning"
        :issues="issues"
        @change="onTurning"
        @inspect="inspected = $event"
      />
      <p v-if="issues.length > 0 || plan?.error" class="problems">
        <template v-if="plan?.error">{{ plan.error }}</template>
        <template v-for="issue in issues" :key="issue.field">{{ issue.field }} {{ issue.problem }}. </template>
      </p>
      <div class="actions">
        <button
          @click="
            settle();
            choosingPreset = true
          "
        >
          Presets…
        </button>
        <button @click="reset">Reset</button>
        <button class="go" :disabled="running" @click="requestPlan">⏵ Start</button>
        <button class="stop" @click="stop">⏹ Stop</button>
      </div>
    </section>

    <section class="figure">
      <BackplotCanvas v-if="liveRun" :plot="liveRun.plot" :start="liveRun.start" :live="liveTool" />
      <CycleDiagram v-else :kind="cycles.selected" :highlight="highlight" />
    </section>

    <ModalDialog
      v-if="choosingPreset"
      :title="cycles.selected === 'threading' ? 'Threading presets' : 'Turning presets'"
      @close="choosingPreset = false"
    >
      <PresetPicker
        v-if="cycles.selected === 'threading'"
        :sections="threadSections"
        :tab="panel.settings?.thread_preset_tab ?? 0"
        @tab="updatePreferences({ thread_preset_tab: $event })"
        @pick="pickThread"
      />
      <PresetPicker
        v-else
        :sections="turningSections"
        :tab="panel.settings?.turning_preset_tab ?? 0"
        @tab="updatePreferences({ turning_preset_tab: $event })"
        @pick="pickTurning"
      />
    </ModalDialog>

    <ModalDialog v-if="previewing && plan?.backplot" :title="`${title}${presetName ? ` · ${presetName}` : ''} — preview`" wide @close="cancelPreview">
      <div class="preview">
        <BackplotCanvas :plot="plan.backplot" :start="plan.start" />
        <aside>
          <ol class="gcode">
            <li v-for="(line, index) in previewRows" :key="index">{{ line }}</li>
          </ol>
          <div class="preview-actions">
            <button @click="cancelPreview">Cancel</button>
            <button class="go" @click="runCycle">⏵ Run cycle</button>
          </div>
        </aside>
      </div>
    </ModalDialog>
  </div>
</template>

<style scoped>
.cycles {
  display: grid;
  grid-template-columns: 1fr 480px;
  gap: 20px;
  height: 100%;
}

.form {
  display: flex;
  flex-direction: column;
  gap: 8px;
  min-height: 0;
}

.form-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--gap);
}

.kinds {
  display: flex;
  gap: 4px;
}

.kinds button {
  height: 34px;
  padding: 0 18px;
  background: transparent;
  border-radius: 0;
  box-shadow: inset 0 -2px 0 var(--line);
  color: var(--text-dim);
}

.kinds button.active {
  color: var(--accent);
  box-shadow: inset 0 -3px 0 var(--accent);
}

.figure {
  min-height: 0;
  padding-left: 20px;
  border-left: 1px solid var(--line);
}

.problems {
  margin: 0;
  color: var(--warn);
  font-weight: 400;
}

.actions {
  display: grid;
  grid-template-columns: repeat(4, 140px);
  justify-content: end;
  gap: var(--gap);
  margin-top: auto;
}

.actions button {
  height: 48px;
}

.preview {
  display: grid;
  grid-template-columns: 1fr 300px;
  gap: 16px;
  height: 100%;
}

aside {
  display: flex;
  flex-direction: column;
  gap: 12px;
  min-height: 0;
}

.gcode {
  flex: 1;
  min-height: 0;
  overflow: auto;
  margin: 0;
  padding: 8px 8px 8px 44px;
  border-radius: var(--radius);
  background: var(--bg-sunken);
  font-weight: 400;
  font-size: 0.85rem;
  color: var(--text-dim);
}

.preview-actions {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: var(--gap);
}

.preview-actions button {
  height: 60px;
}
.preset-name {
  overflow: hidden;
  color: var(--text-dim);
  font-size: 0.85rem;
  white-space: nowrap;
  text-overflow: ellipsis;
}
</style>
