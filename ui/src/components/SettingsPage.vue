<script setup lang="ts">
import { computed } from 'vue'

import { activeField, entryText, select, settle } from '../entry'
import { panel, updatePreferences } from '../machine'
import NumPad from './NumPad.vue'

const settings = computed(() => panel.settings)

function choose(change: Parameters<typeof updatePreferences>[0]): void {
  settle()
  updatePreferences(change)
}

function editScale(axis: 'z' | 'x'): void {
  select({
    id: `settings.scale.${axis}`,
    current: () =>
      axis === 'z'
        ? (panel.settings?.machine.encoder_scale_z ?? 0)
        : (panel.settings?.machine.encoder_scale_x ?? 0),
    commit: (value) => {
      updatePreferences(axis === 'z' ? { encoder_scale_z: value } : { encoder_scale_x: value })
    },
  })
}

function shownScale(axis: 'z' | 'x'): string {
  const typed = entryText(`settings.scale.${axis}`)
  if (typed !== null) {
    return typed
  }
  const value =
    axis === 'z' ? settings.value?.machine.encoder_scale_z : settings.value?.machine.encoder_scale_x
  return (value ?? 0).toFixed(6)
}
</script>

<template>
  <div v-if="settings" class="settings">
    <div class="rows">
      <h1>Settings</h1>
      <span class="label">X display</span>
      <div class="choice">
        <button class="option" :class="{ selected: !settings.diameter_mode }" @click="choose({ diameter_mode: false })">
          <span class="dot"></span>Radius
        </button>
        <button class="option" :class="{ selected: settings.diameter_mode }" @click="choose({ diameter_mode: true })">
          <span class="dot"></span>Diameter
        </button>
      </div>

      <span class="label">Tool post</span>
      <div class="choice">
        <button class="option" :class="{ selected: !settings.back_tool_post }" @click="choose({ back_tool_post: false })">
          <span class="dot"></span>Front
        </button>
        <button class="option" :class="{ selected: settings.back_tool_post }" @click="choose({ back_tool_post: true })">
          <span class="dot"></span>Back
        </button>
      </div>

      <span class="label">Units at start</span>
      <div class="choice">
        <button
          class="option"
          :class="{ selected: settings.metric_on_startup }"
          @click="choose({ metric_on_startup: true })"
        >
          <span class="dot"></span>Metric (mm)
        </button>
        <button
          class="option"
          :class="{ selected: !settings.metric_on_startup }"
          @click="choose({ metric_on_startup: false })"
        >
          <span class="dot"></span>Imperial (in)
        </button>
      </div>

      <span class="label">Encoder scale Z</span>
      <button class="scale" :class="{ entering: activeField === 'settings.scale.z' }" @click="editScale('z')">
        {{ shownScale('z') }}
      </button>

      <span class="label">Encoder scale X</span>
      <button class="scale" :class="{ entering: activeField === 'settings.scale.x' }" @click="editScale('x')">
        {{ shownScale('x') }}
      </button>
      <p class="hint">mm per encoder count. The sign sets the counting direction.</p>
    </div>
    <NumPad />
  </div>
</template>

<style scoped>
.settings {
  display: grid;
  grid-template-columns: 1fr auto;
  gap: 40px;
  padding: 24px;
}

.rows {
  display: grid;
  grid-template-columns: 200px 1fr;
  gap: 14px 20px;
  align-content: start;
  align-items: center;
}

h1 {
  grid-column: 1 / -1;
  margin: 0 0 8px;
  font-size: 1.4rem;
}

.label {
  color: var(--text-dim);
  text-align: right;
}

.choice {
  display: grid;
  grid-template-columns: repeat(2, 220px);
  gap: var(--gap);
}

.scale {
  width: 220px;
  height: 52px;
  padding: 0 14px;
  text-align: left;
  font-variant-numeric: tabular-nums;
}

.scale.entering {
  color: var(--entry);
  box-shadow: inset 3px 0 0 var(--entry);
}

.hint {
  grid-column: 2;
  margin: 0;
  font-weight: 400;
  color: var(--text-dim);
}
</style>
