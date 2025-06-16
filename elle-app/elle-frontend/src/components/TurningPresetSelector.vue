<template>
  <TabView v-model:active-index="selectedTurningTab">
    <TabPanel
      v-for="(section, sindex) in sections"
      :key="sindex"
      :value="sindex"
      :header="section.header"
    >
      <div class="grid dro-font-preset-button">
        <template v-for="(preset, pindex) in section.presets" :key="preset.name">
          <!-- Add visual separator before first internal thread -->
          <template v-if="shouldShowSeparator(section, pindex)">
            <div class="col-12 p-2">
              <div class="thread-separator"></div>
            </div>
          </template>
          <div class="col-3 p-1">
            <button class="w-full h-full button-pitchselector" @click="presetClicked(preset)">
              {{ preset.name }}
            </button>
          </div>
        </template>
      </div>
    </TabPanel>
  </TabView>
</template>

<script setup lang="ts">
import { inject } from 'vue'
import turningPresetsData from '../assets/turningpresets.json'
import { useSettings } from '../composables/useSettings'

const emit = defineEmits(['selected'])
const dialogRef = inject('dialogRef') as any
const { selectedTurningTab } = useSettings()

interface TurningPreset {
  name: string
  description: string
  // External thread properties
  stockDiameter?: number
  cuttingDepth?: number
  // Internal thread properties
  drillDiameter?: number
  boringDepth?: number
  // Common properties
  target: number
  stock: number
  zEnd: number
  feedRate: number
  finalStepDown: number
  taperAngle: number
  stepDown: number
}

const presets = turningPresetsData

const sections = [
  { header: 'Metric', presets: presets.metric },
  { header: 'Imperial', presets: presets.imperial },
  { header: 'NPT', presets: presets.npt }
]

const presetClicked = (preset: TurningPreset) => {
  emit('selected', preset)
  dialogRef.value.close()
}

// Helper function to determine if we should show separator before this preset
const shouldShowSeparator = (section: any, pindex: number): boolean => {
  const presets = section.presets

  if (pindex > 0 && pindex < presets.length) {
    const currentPreset = presets[pindex]
    const previousPreset = presets[pindex - 1]

    // External thread pattern: -2A
    const isPreviousExternal = previousPreset.name.includes('-2A')

    // Internal thread pattern: -2B
    const isCurrentInternal = currentPreset.name.includes('-2B')

    return isPreviousExternal && isCurrentInternal
  }

  return false
}
</script>

<style>
.button-pitchselector {
  background: #333;
}

.p-tabview {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.p-tabview-panels {
  flex-grow: 10;
}

.dro-font-preset-button {
  font-family: 'iosevka';
  font-weight: bold;
  font-size: 1.3125em;
  text-align: center;
}

.thread-separator {
  border-top: 1px solid #555;
  margin: 0.5rem 0;
}
</style>
