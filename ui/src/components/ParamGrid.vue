<script setup lang="ts" generic="Key extends string">
// Two-column grid of cycle parameters: tap a value to enter it on the numpad, tap a label for help.
import { computed, ref, useTemplateRef } from 'vue'

import type { ParamField } from '../cycles'
import { activeField, entryText, select } from '../entry'
import { panel } from '../machine'
import type { ParameterIssue } from '../protocol'
import { lengthDecimals, lengthFromDisplay, lengthToDisplay, xFromDisplay, xToDisplay } from '../units'

const props = defineProps<{
  prefix: string
  fields: (ParamField<Key> | null)[]
  values: Record<Key, number | null>
  issues: readonly ParameterIssue[]
  /** Shown in the first empty slot, e.g. the thread's major diameter. */
  info?: string
}>()
const emit = defineEmits<{ change: [key: Key, value: number | null] }>()

const metric = computed(() => panel.metric)
const diameterMode = computed(() => panel.settings?.diameter_mode ?? false)

function toDisplay(field: ParamField<Key>, mm: number): number {
  switch (field.kind) {
    case 'length':
      return lengthToDisplay(mm, metric.value)
    case 'diameter':
      return xToDisplay(mm, metric.value, diameterMode.value)
    case 'angle':
    case 'ratio':
    case 'count':
      return mm
  }
}

function fromDisplay(field: ParamField<Key>, value: number): number {
  switch (field.kind) {
    case 'length':
      return lengthFromDisplay(value, metric.value)
    case 'diameter':
      return xFromDisplay(value, metric.value, diameterMode.value)
    case 'angle':
    case 'ratio':
    case 'count':
      return value
  }
}

function decimals(field: ParamField<Key>): number {
  switch (field.kind) {
    case 'length':
    case 'diameter':
      return lengthDecimals(metric.value)
    case 'angle':
      return 4
    case 'ratio':
      return 3
    case 'count':
      return 0
  }
}

function entryId(field: ParamField<Key>): string {
  return `${props.prefix}.${field.key}`
}

function edit(field: ParamField<Key>): void {
  select({
    id: entryId(field),
    current: () => {
      const value = props.values[field.key]
      return value === null ? null : toDisplay(field, value)
    },
    commit: (value) => {
      emit('change', field.key, fromDisplay(field, value))
    },
    clear: () => {
      emit('change', field.key, null)
    },
  })
}

function shown(field: ParamField<Key>): string {
  const typed = entryText(entryId(field))
  if (typed !== null) {
    return typed
  }
  const value = props.values[field.key]
  return value === null ? field.placeholder : String(Number(toDisplay(field, value).toFixed(decimals(field))))
}

function flagged(field: ParamField<Key>): ParameterIssue | undefined {
  return props.issues.find((issue) => issue.field === field.key)
}

const help = useTemplateRef<HTMLDivElement>('help')
const helpText = ref('')
let helpField = ''
function showHelp(field: ParamField<Key>, event: MouseEvent): void {
  const popover = help.value
  if (!popover) {
    return
  }
  if (helpField === field.key && popover.matches(':popover-open')) {
    popover.hidePopover()
    helpField = ''
    return
  }
  helpField = field.key
  helpText.value = field.help
  popover.style.left = `${String(Math.min(event.clientX, window.innerWidth - 340))}px`
  popover.style.top = `${String(event.clientY + 16)}px`
  popover.showPopover()
}
</script>

<template>
  <div class="grid">
    <template v-for="(field, index) in fields" :key="field?.key ?? `empty-${String(index)}`">
      <template v-if="field">
        <button class="label" @click="showHelp(field, $event)">{{ field.label }}</button>
        <button
          class="value"
          :class="{
            entering: activeField === entryId(field),
            empty: values[field.key] === null && activeField !== entryId(field),
            flagged: flagged(field) !== undefined,
          }"
          :title="flagged(field)?.problem"
          @click="edit(field)"
        >
          {{ shown(field) }}
        </button>
      </template>
      <template v-else-if="index === 1 && info !== undefined">
        <span class="label">Ø</span>
        <span class="value readonly">{{ info || 'Major Ø / Drill' }}</span>
      </template>
      <template v-else>
        <span></span>
        <span></span>
      </template>
    </template>
    <div ref="help" class="help" popover="auto">{{ helpText }}</div>
  </div>
</template>

<style scoped>
.grid {
  display: grid;
  grid-template-columns: 84px 1fr 84px 1fr;
  grid-auto-rows: 40px;
  gap: 5px 8px;
}

.label {
  /* Labels are buttons (with help) or, for Ø, plain text: centre both the same way. */
  display: flex;
  align-items: center;
  justify-content: flex-end;
  background: transparent;
  text-align: right;
  padding-right: 6px;
  color: var(--text-dim);
}

.value {
  display: flex;
  align-items: center;
  padding: 0 12px;
  text-align: left;
  overflow: hidden;
  white-space: nowrap;
  text-overflow: ellipsis;
  font-variant-numeric: tabular-nums;
}

.value.empty {
  color: var(--text-faint);
}

.value.entering {
  background: rgb(239 68 68 / 14%);
  box-shadow: inset 3px 0 0 var(--entry);
  color: var(--entry);
}

.value.flagged {
  box-shadow: inset 0 0 0 2px var(--warn);
}

.value.readonly {
  border-radius: var(--radius);
  background: var(--bg-sunken);
  color: var(--text-dim);
}

.help {
  position: fixed;
  inset: auto;
  max-width: 320px;
  margin: 0;
  padding: 10px 12px;
  border: 1px solid var(--line);
  border-radius: var(--radius);
  background: #2b2b2b;
  color: var(--text);
  font-weight: 400;
  white-space: pre-line;
}
</style>
