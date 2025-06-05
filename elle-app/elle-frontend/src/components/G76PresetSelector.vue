<script setup lang="ts">
import { inject } from "vue";
import presetsData from "../assets/g76presets.json";

const emit = defineEmits(["selected"]);
const dialogRef = inject("dialogRef") as any;

interface G76Preset {
  name: string;
  description: string;
  P: number;  // Thread pitch
  Z: number;  // End point
  I: number;  // Thread peak offset
  J: number;  // Initial cut depth
  R: number;  // Depth degression factor
  K: number;  // Full thread depth
  Q: number;  // Compound slide angle
  H: number;  // Spring passes
  E: number;  // Taper distance
  L: number;  // Taper end specification
}

const presets = presetsData;

const sections = [
  { header: "Metric", presets: presets.metric },
  { header: "Imperial", presets: presets.imperial }, 
  { header: "Special", presets: presets.special }
];

const presetClicked = (preset: G76Preset) => {
  emit("selected", preset);
  dialogRef.value.close();
};
</script>

<template>
  <TabView>
    <TabPanel v-for="(section, sindex) in sections" :key="sindex" :value="sindex" :header="section.header">
      <div class="grid dro-font-preset-button">
        <div
          class="col-3 p-1"
          v-for="(preset, pindex) in section.presets"
          :key="pindex"
        >
          <button
            @click="presetClicked(preset)"
            class="w-full h-full button-pitchselector"
          >
            {{ preset.name }}
          </button>
          <br />
        </div>
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
  font-family: "iosevka";
  font-weight: bold;
  font-size: 1.75em;
  text-align: center;
}
</style>