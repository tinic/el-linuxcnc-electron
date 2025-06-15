<template>
  <div class="turning-preset-selector">
    <div class="grid dro-font-preset-button">
      <template v-for="(preset, pindex) in turningPresets" :key="preset.name">
        <!-- Add visual separator before first internal thread -->
        <template v-if="shouldShowSeparator(pindex)">
          <div class="col-12 p-2">
            <div class="thread-separator"></div>
          </div>
        </template>
        <div class="col-3 p-1">
          <button
            @click="presetClicked(preset)"
            class="w-full h-full button-pitchselector"
          >
            {{ preset.name }}
          </button>
        </div>
      </template>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, inject } from 'vue';
import turningPresetsData from '../assets/turningpresets.json';

const emit = defineEmits(['selected']);
const dialogRef = inject("dialogRef") as any;

interface TurningPreset {
  name: string;
  description: string;
  // External thread properties
  stockDiameter?: number;
  cuttingDepth?: number;
  // Internal thread properties  
  drillDiameter?: number;
  boringDepth?: number;
  // Common properties
  target: number;
  stock: number;
  zStart: number;
  zEnd: number;
  feedRate: number;
  finalStepDown: number;
  taperAngle: number;
  stepDown: number;
}

const turningPresets = ref<TurningPreset[]>([]);

onMounted(() => {
  turningPresets.value = turningPresetsData.npt || [];
});

const presetClicked = (preset: TurningPreset) => {
  emit('selected', preset);
  dialogRef.value.close();
};

// Helper function to determine if we should show separator before this preset
const shouldShowSeparator = (pindex: number): boolean => {
  if (pindex > 0 && pindex < turningPresets.value.length) {
    const currentPreset = turningPresets.value[pindex];
    const previousPreset = turningPresets.value[pindex - 1];
    
    // External thread pattern: -2A
    const isPreviousExternal = previousPreset.name.includes('-2A');
    
    // Internal thread pattern: -2B
    const isCurrentInternal = currentPreset.name.includes('-2B');
    
    return isPreviousExternal && isCurrentInternal;
  }
  
  return false;
};
</script>

<style>
.button-pitchselector {
  background: #333;
}

.dro-font-preset-button {
  font-family: "iosevka";
  font-weight: bold;
  font-size: 1.3125em;
  text-align: center;
}

.thread-separator {
  border-top: 1px solid #555;
  margin: 0.5rem 0;
}
</style>