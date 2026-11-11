<script setup lang="ts">
import { press, type Key } from '../entry'

interface Pad {
  key: Key
  label: string
  area: string
}

// Same keys and arrangement as the original panel.
const pads: Pad[] = [
  { key: 'escape', label: 'ESC', area: 'esc' },
  { key: 'third', label: '⅓', area: 'third' },
  { key: 'half', label: '½', area: 'half' },
  { key: 'backspace', label: '⌫', area: 'back' },
  { key: '7', label: '7', area: 'n7' },
  { key: '8', label: '8', area: 'n8' },
  { key: '9', label: '9', area: 'n9' },
  { key: 'sign', label: '±', area: 'sign' },
  { key: '4', label: '4', area: 'n4' },
  { key: '5', label: '5', area: 'n5' },
  { key: '6', label: '6', area: 'n6' },
  { key: '1', label: '1', area: 'n1' },
  { key: '2', label: '2', area: 'n2' },
  { key: '3', label: '3', area: 'n3' },
  { key: 'enter', label: 'ENT', area: 'ent' },
  { key: '0', label: '0', area: 'n0' },
  { key: '.', label: '.', area: 'dot' },
]
</script>

<template>
  <div class="numpad">
    <button
      v-for="pad in pads"
      :key="pad.key"
      :style="{ gridArea: pad.area }"
      :class="{ ent: pad.key === 'enter' }"
      @pointerdown.prevent="press(pad.key)"
    >
      {{ pad.label }}
    </button>
  </div>
</template>

<style scoped>
.numpad {
  display: grid;
  grid-template-columns: repeat(4, 76px);
  grid-template-rows: repeat(5, 60px);
  grid-template-areas:
    'esc third half back'
    'n7 n8 n9 sign'
    'n4 n5 n6 sign'
    'n1 n2 n3 ent'
    'n0 n0 dot ent';
  gap: var(--gap);
}

.numpad button {
  font-size: 1.25rem;
}

.numpad .ent {
  background: var(--accent);
}
</style>
