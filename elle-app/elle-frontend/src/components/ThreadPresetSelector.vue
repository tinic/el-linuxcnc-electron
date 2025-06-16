<script setup lang="ts">
import { inject } from 'vue'
import presetsData from '../assets/threadpresets.json'

const emit = defineEmits(['selected'])
const dialogRef = inject('dialogRef') as any

interface ThreadPreset {
  name: string
  description: string
  Pitch: number // Pitch
  Diameter: string // Major Diameter / Drill Size
  XDepth: number // X Depth
  ZDepth: number // Z Depth
  Angle: number // Thread angle in degrees (0 for straight threads, angle for tapered threads)
  ZEnd: number // Z End
  XPullout: number // X Pullout
  ZPullout: number // Z Pullout
  FirstCut: number // First Cut
  CutMult: number // Cut Multiplier
  MinCut: number // Min Cut
  SpringCuts: number // Spring Cuts
}

const presets = presetsData

const sections = [
  { header: 'Metric', presets: presets.metric },
  { header: 'Imperial', presets: presets.imperial },
  { header: 'NPT', presets: presets.special }
]

const presetClicked = (preset: ThreadPreset) => {
  emit('selected', preset)
  dialogRef.value.close()
}

// Helper function to determine if we should show separator before this preset
const shouldShowSeparator = (section: any, pindex: number): boolean => {
  const presets = section.presets

  // Show separator before the first internal thread
  if (pindex > 0 && pindex < presets.length) {
    const currentPreset = presets[pindex]
    const previousPreset = presets[pindex - 1]

    // External thread patterns: 6g, 2A (including NPT-2A)
    const isPreviousExternal =
      previousPreset.name.includes('6g') || previousPreset.name.includes('-2A')

    // Internal thread patterns: 6H, 2B (including NPT-2B)
    const isCurrentInternal =
      currentPreset.name.includes('6H') || currentPreset.name.includes('-2B')

    return isPreviousExternal && isCurrentInternal
  }

  return false
}
</script>

<template>
  <TabView>
    <TabPanel
      v-for="(section, sindex) in sections"
      :key="sindex"
      :value="sindex"
      :header="section.header"
    >
      <div class="grid dro-font-preset-button">
        <!-- eslint-disable-next-line vue/no-unused-vars -->
        <template v-for="(preset, pindex) in section.presets" :key="pindex">
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
            <br />
          </div>
        </template>
      </div>
    </TabPanel>
  </TabView>
</template>

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
