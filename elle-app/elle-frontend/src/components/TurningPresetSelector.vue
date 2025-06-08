<script setup lang="ts">
import { inject } from "vue";
import presetsData from "../assets/turningpresets.json";

const emit = defineEmits(["selected"]);
const dialogRef = inject("dialogRef") as any;

interface TurningPreset {
  name: string;
  description: string;
  outerDiameter: string;
  drillSize: string;
  xStart: number;
  xEnd: number;
  zStart: number;
  zEnd: number;
  feedRate: number;
  roughingPasses: number;
  finishingAllowance: number;
  taperAngle: number;
  stepDown: number;
}

const presets = presetsData;

// Function to determine if we need a separator before this preset
const needsSeparator = (preset: TurningPreset, index: number, allPresets: TurningPreset[]) => {
  if (index === 0) return false;
  
  const currentType = preset.name.includes('-2A') ? 'External' : 'Internal';
  const previousType = allPresets[index - 1].name.includes('-2A') ? 'External' : 'Internal';
  
  return currentType !== previousType;
};

// Function to get separator text
const getSeparatorText = (preset: TurningPreset) => {
  return preset.name.includes('-2A') ? 'External NPT' : '';
};

const presetClicked = (preset: TurningPreset) => {
  emit("selected", preset);
  dialogRef.value.close();
};
</script>

<template>
  <div class="grid dro-font-preset-button">
    <template v-for="(preset, pindex) in presets.npt" :key="pindex">
      <!-- Add separator before External presets -->
      <div v-if="needsSeparator(preset, pindex, presets.npt)" class="col-12 p-1">
        <div class="preset-separator">
          {{ getSeparatorText(preset) }}
        </div>
      </div>
      
      <div class="col-6 p-1">
        <button
          @click="presetClicked(preset)"
          class="w-full h-full button-pitchselector preset-button"
        >
          <div class="preset-content">
            <div class="preset-name">{{ preset.name }}</div>
            <div class="preset-details">
              <div class="preset-info">⌽{{ preset.outerDiameter }}mm</div>
              <div class="preset-drill">{{ preset.drillSize }}</div>
            </div>
          </div>
        </button>
        <br />
      </div>
    </template>
  </div>
</template>

<style scoped>
.button-pitchselector {
  background: #333;
  border: 1px solid #555;
  transition: all 0.2s ease;
}

.button-pitchselector:hover {
  background: #444;
  border-color: #666;
}

.preset-button {
  padding: 12px;
  min-height: 80px;
}

.preset-content {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 6px;
}

.preset-name {
  font-weight: bold;
  font-size: 1.1em;
  color: #ffffff;
  text-align: center;
}

.preset-details {
  display: flex;
  flex-direction: column;
  gap: 2px;
  text-align: center;
}

.preset-info {
  font-size: 0.85em;
  color: #aaaaaa;
  font-weight: normal;
}

.preset-drill {
  font-size: 0.75em;
  color: #888888;
  font-weight: normal;
  font-style: italic;
}

.preset-separator {
  text-align: center;
  font-size: 1.0em;
  font-weight: bold;
  color: #ffffff;
  background: rgba(255, 255, 255, 0.1);
  border: 1px solid rgba(255, 255, 255, 0.2);
  border-radius: 4px;
  padding: 8px 12px;
  margin: 8px 0;
}

.dro-font-preset-button {
  font-family: "iosevka";
  font-weight: bold;
  font-size: 1.2em;
  text-align: center;
}
</style>