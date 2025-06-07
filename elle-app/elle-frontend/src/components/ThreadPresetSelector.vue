<script setup lang="ts">
import { inject } from "vue";
import presetsData from "../assets/threadpresets.json";

const emit = defineEmits(["selected"]);
const dialogRef = inject("dialogRef") as any;

interface ThreadPreset {
  name: string;
  description: string;
  Pitch: number;     // Pitch
  XDepth: number;    // X Depth
  ZDepth: number;    // Z Depth
  Angle: number;     // Thread angle in degrees (0 for straight threads, angle for tapered threads)
  ZEnd: number;      // Z End
  XPullout: number;  // X Pullout
  ZPullout: number;  // Z Pullout
  FirstCut: number;  // First Cut
  CutMult: number;   // Cut Multiplier
  MinCut: number;    // Min Cut
  SpringCuts: number;// Spring Cuts
}

const presets = presetsData;

const sections = [
  { header: "Metric", presets: presets.metric },
  { header: "Imperial", presets: presets.imperial }, 
  { header: "Special", presets: presets.special }
];

const presetClicked = (preset: ThreadPreset) => {
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