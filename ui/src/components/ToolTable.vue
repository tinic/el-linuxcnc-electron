<script setup lang="ts">
// Tool offsets, entered on the numpad (the original needed a keyboard). Offsets are relative to
// the reference tool T00: X as a radius (diameter in diameter mode), Z in length units.
import { computed } from 'vue'

import { activeField, entryText, select } from '../entry'
import { panel, send } from '../machine'
import type { Tool } from '../protocol'
import { lengthDecimals, lengthFromDisplay, lengthToDisplay, xFromDisplay, xToDisplay } from '../units'
import NumPad from './NumPad.vue'

const emit = defineEmits<{ close: [] }>()

const tools = computed(() => panel.settings?.machine.tools ?? [])
const current = computed(() => panel.settings?.machine.current_tool ?? 0)
const metric = computed(() => panel.metric)
const diameter = computed(() => panel.settings?.diameter_mode ?? false)

function update(id: number, change: Partial<Tool>): void {
  send({
    type: 'set_tools',
    tools: tools.value.map((tool) => (tool.id === id ? { ...tool, ...change } : { ...tool })),
  })
}

function editOffset(tool: Tool, axis: 'x' | 'z'): void {
  select({
    id: `tool.${String(tool.id)}.${axis}`,
    current: () => {
      const latest = tools.value.find((t) => t.id === tool.id) ?? tool
      return axis === 'x'
        ? xToDisplay(latest.offset_x, metric.value, diameter.value)
        : lengthToDisplay(latest.offset_z, metric.value)
    },
    commit: (value) => {
      update(
        tool.id,
        axis === 'x'
          ? { offset_x: xFromDisplay(value, metric.value, diameter.value) }
          : { offset_z: lengthFromDisplay(value, metric.value) },
      )
    },
  })
}

function shown(tool: Tool, axis: 'x' | 'z'): string {
  const typed = entryText(`tool.${String(tool.id)}.${axis}`)
  if (typed !== null) {
    return typed
  }
  const value =
    axis === 'x'
      ? xToDisplay(tool.offset_x, metric.value, diameter.value)
      : lengthToDisplay(tool.offset_z, metric.value)
  return value.toFixed(lengthDecimals(metric.value))
}

function choose(tool: Tool): void {
  send({ type: 'select_tool', id: tool.id })
  emit('close')
}

function rename(tool: Tool, event: Event): void {
  update(tool.id, { description: (event.target as HTMLInputElement).value })
}
</script>

<template>
  <div class="tools">
    <table>
      <thead>
        <tr>
          <th>Tool</th>
          <th>{{ diameter ? '⌀X' : 'X' }} ({{ metric ? 'mm' : 'in' }})</th>
          <th>Z ({{ metric ? 'mm' : 'in' }})</th>
          <th>Description</th>
        </tr>
      </thead>
      <tbody>
        <tr v-for="tool in tools" :key="tool.id" :class="{ current: tool.id === current }">
          <td>
            <button class="pick" @click="choose(tool)">
              <span class="dot"></span>T{{ String(tool.id).padStart(2, '0') }}
            </button>
          </td>
          <td>
            <button
              class="cell"
              :class="{ entering: activeField === `tool.${String(tool.id)}.x` }"
              :disabled="tool.id === 0"
              @click="editOffset(tool, 'x')"
            >
              {{ shown(tool, 'x') }}
            </button>
          </td>
          <td>
            <button
              class="cell"
              :class="{ entering: activeField === `tool.${String(tool.id)}.z` }"
              :disabled="tool.id === 0"
              @click="editOffset(tool, 'z')"
            >
              {{ shown(tool, 'z') }}
            </button>
          </td>
          <td><input :value="tool.description" @change="rename(tool, $event)" /></td>
        </tr>
      </tbody>
    </table>
    <NumPad />
  </div>
</template>

<style scoped>
.tools {
  display: grid;
  grid-template-columns: 1fr auto;
  gap: 24px;
  align-items: start;
}

table {
  width: 100%;
  border-collapse: separate;
  border-spacing: 0 6px;
}

th {
  text-align: left;
  font-size: 0.8rem;
  letter-spacing: 0.08em;
  color: var(--text-dim);
  padding: 0 6px;
}

td {
  padding: 0 4px;
}

.pick,
.cell {
  width: 100%;
  height: 44px;
  padding: 0 12px;
  text-align: left;
  font-variant-numeric: tabular-nums;
}

.pick {
  display: flex;
  align-items: center;
  gap: 10px;
}

.dot {
  width: 14px;
  height: 14px;
  border-radius: 50%;
  border: 2px solid var(--text-dim);
}

tr.current .pick {
  background: var(--accent-soft);
  box-shadow: inset 0 0 0 1px var(--accent);
}

tr.current .dot {
  border-color: var(--accent);
  background: var(--accent);
}

.cell.entering {
  color: var(--entry);
  box-shadow: inset 3px 0 0 var(--entry);
}

input {
  width: 100%;
}
</style>
