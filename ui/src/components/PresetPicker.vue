<script setup lang="ts" generic="Item extends { name: string }">
// Tabbed grid of preset buttons, used for pitches, threads and turning presets.
import { computed } from 'vue'

import type { PresetSection } from '../presets'

const props = defineProps<{ sections: PresetSection<Item>[]; tab: number }>()
const emit = defineEmits<{ pick: [item: Item]; tab: [index: number] }>()

const current = computed(() => props.sections[Math.min(props.tab, props.sections.length - 1)])
</script>

<template>
  <div class="picker">
    <nav>
      <button
        v-for="(section, index) in sections"
        :key="section.header"
        :class="{ active: section === current }"
        @click="emit('tab', index)"
      >
        {{ section.header }}
      </button>
    </nav>
    <div v-if="current" class="grid">
      <template v-for="(item, index) in current.items" :key="`${String(index)}-${item.name}`">
        <hr v-if="current.splitAt !== undefined && index === current.splitAt" />
        <button class="preset" @click="emit('pick', item)">{{ item.name }}</button>
      </template>
    </div>
  </div>
</template>

<style scoped>
nav {
  display: flex;
  gap: var(--gap);
  margin-bottom: 14px;
}

nav button {
  height: 48px;
  padding: 0 20px;
  background: transparent;
  box-shadow: inset 0 -2px 0 var(--line);
  border-radius: 0;
}

nav button.active {
  color: var(--accent);
  box-shadow: inset 0 -3px 0 var(--accent);
}

.grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: var(--gap);
}

.preset {
  min-height: 56px;
  padding: 8px;
}

hr {
  grid-column: 1 / -1;
  width: 100%;
  border: 0;
  border-top: 1px solid var(--line);
  margin: 6px 0;
}
</style>
