<script setup lang="ts">
import { computed } from 'vue'

import { activeField, entryText, select, settle, type EntryField } from '../entry'
import { panel, send, setPitch, toggleUnits } from '../machine'
import {
  lengthDecimals,
  lengthFromDisplay,
  lengthToDisplay,
  xFromDisplay,
  xToDisplay,
} from '../units'
import AppIcon from './AppIcon.vue'

defineProps<{ showPitches: boolean }>()
const emit = defineEmits<{ pitchPresets: [axis: 'x' | 'z']; toolTable: [] }>()

const metric = computed(() => panel.metric)
const diameter = computed(() => panel.settings?.diameter_mode ?? false)
const readout = computed(() => panel.status?.readout ?? { x: 0, z: 0, a_degrees: 0, rpm: 0 })
const pitches = computed(() => panel.status?.pitches ?? { x: 0, z: 0 })
const leadscrew = computed(() => panel.status?.leadscrew)
const tool = computed(() => panel.settings?.machine.current_tool ?? 0)

function enterPitch(axis: 'x' | 'z', value: number): void {
  setPitch(axis, lengthFromDisplay(value, metric.value))
}

const fields: Record<'x' | 'z' | 'a' | 'px' | 'pz', EntryField> = {
  x: {
    id: 'dro.x',
    current: () => xToDisplay(readout.value.x, metric.value, diameter.value),
    commit: (value) => {
      send({ type: 'set_axis', axis: 'x', value: xFromDisplay(value, metric.value, diameter.value) })
    },
  },
  z: {
    id: 'dro.z',
    current: () => lengthToDisplay(readout.value.z, metric.value),
    commit: (value) => {
      send({ type: 'set_axis', axis: 'z', value: lengthFromDisplay(value, metric.value) })
    },
  },
  a: {
    id: 'dro.a',
    current: () => readout.value.a_degrees,
    commit: (value) => {
      send({ type: 'set_axis', axis: 'a', value })
    },
  },
  px: {
    id: 'dro.px',
    current: () => lengthToDisplay(pitches.value.x, metric.value),
    commit: (value) => {
      enterPitch('x', value)
    },
  },
  pz: {
    id: 'dro.pz',
    current: () => lengthToDisplay(pitches.value.z, metric.value),
    commit: (value) => {
      enterPitch('z', value)
    },
  },
}

interface Row {
  key: keyof typeof fields
  label: string
  value: number
  decimals: number
  unit: string
}

const rows = computed<Row[]>(() => {
  const decimals = lengthDecimals(metric.value)
  const length = metric.value ? 'mm' : 'in'
  const pitch = metric.value ? 'mm/rev' : 'in/rev'
  return [
    {
      key: 'x',
      label: diameter.value ? '⌽X' : 'X',
      value: xToDisplay(readout.value.x, metric.value, diameter.value),
      decimals,
      unit: length,
    },
    { key: 'z', label: 'Z', value: lengthToDisplay(readout.value.z, metric.value), decimals, unit: length },
    { key: 'a', label: 'A', value: readout.value.a_degrees, decimals: 3, unit: '°' },
    { key: 'px', label: 'PX', value: lengthToDisplay(pitches.value.x, metric.value), decimals, unit: pitch },
    { key: 'pz', label: 'PZ', value: lengthToDisplay(pitches.value.z, metric.value), decimals, unit: pitch },
  ]
})

function shown(row: Row): string {
  return entryText(fields[row.key].id) ?? row.value.toFixed(row.decimals)
}

function zero(axis: 'x' | 'z' | 'a'): void {
  settle()
  send({ type: 'set_axis', axis, value: 0 })
}

function units(): void {
  settle()
  toggleUnits()
}
</script>

<template>
  <div class="dro">
    <template v-for="row in rows" :key="row.key">
      <div
        v-if="row.key === 'x' || row.key === 'z' || row.key === 'a' || showPitches"
        class="row"
      >
        <button
          class="reading"
          :class="{ entering: activeField === fields[row.key].id }"
          @click="select(fields[row.key])"
        >
          <span class="label">{{ row.label }}</span>
          <span class="value">{{ shown(row) }}</span>
          <span class="unit">{{ row.unit }}</span>
        </button>
        <button v-if="row.key === 'x'" class="action" @click="zero('x')">X₀</button>
        <button v-else-if="row.key === 'z'" class="action" @click="zero('z')">Z₀</button>
        <button v-else-if="row.key === 'a'" class="action" @click="zero('a')">A₀</button>
        <button
          v-else
          class="action"
          @click="
            settle();
            emit('pitchPresets', row.key === 'px' ? 'x' : 'z')
          "
        >
          {{ row.key === 'px' ? panel.pitchLabels.x : panel.pitchLabels.z }}
        </button>
        <div v-if="row.key === 'x' || row.key === 'z'" class="state">
          <span
            :class="{
              on: row.key === 'x' ? leadscrew?.x_driver_enabled : leadscrew?.z_driver_enabled,
            }"
            title="Stepper driver powered (axis locked)"
          >
            <AppIcon name="lock" :size="26" />
          </span>
          <span
            :class="{
              on: row.key === 'x' ? leadscrew?.x_follows_spindle : leadscrew?.z_follows_spindle,
            }"
            title="Following the spindle"
          >
            <AppIcon name="gear" :size="26" />
          </span>
        </div>
      </div>
      <div v-else class="row placeholder"></div>
    </template>
    <div class="row">
      <div class="reading static">
        <span class="label">R</span>
        <span class="value">{{ readout.rpm.toFixed(0) }}</span>
        <span class="unit">rpm</span>
      </div>
      <button class="action" @click="units">{{ metric ? 'mm' : 'in' }} ⇄ {{ metric ? 'in' : 'mm' }}</button>
      <button
        class="action tool"
        @click="
          settle();
          emit('toolTable')
        "
      >
        <AppIcon name="tool" :size="20" />
        T{{ String(tool).padStart(2, '0') }}
      </button>
    </div>
  </div>
</template>

<style scoped>
.dro {
  display: grid;
  grid-auto-rows: 52px;
  gap: 6px;
}

.row {
  display: grid;
  grid-template-columns: 390px 120px auto;
  gap: var(--gap);
  align-items: stretch;
}

.reading {
  display: grid;
  grid-template-columns: 64px 1fr 112px;
  align-items: baseline;
  padding: 0 12px;
  background: transparent;
  font-size: 2.05rem;
  text-align: right;
  font-variant-numeric: tabular-nums;
}

.reading.static {
  display: grid;
}

.reading .label {
  /* Right-aligned against the value, as in the original DRO (" Z|", "PX|", "⌽X|"). */
  text-align: right;
  padding-right: 10px;
  color: var(--text-dim);
}

.reading .unit {
  /* Same size as the number, as in the original DRO, so ° and the units sit on its baseline. */
  padding-left: 8px;
  text-align: left;
  color: var(--text-dim);
}

.reading.entering {
  background: rgb(239 68 68 / 12%);
  box-shadow: inset 3px 0 0 var(--entry);
}

.reading.entering .label,
.reading.entering .value {
  color: var(--entry);
}

.action {
  font-size: 1.05rem;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.tool {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  min-width: 120px;
}

.state {
  display: flex;
  align-items: center;
  gap: 14px;
  padding-left: 8px;
  color: var(--text-faint);
}

.state .on {
  color: var(--entry);
}
</style>
