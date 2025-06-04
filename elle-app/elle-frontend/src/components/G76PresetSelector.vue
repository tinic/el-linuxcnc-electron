<script setup lang="ts">
import { inject } from "vue";

const emit = defineEmits(["selected"]);
const dialogRef = inject("dialogRef") as any;

interface G76Preset {
  name: string;
  X: number;
  Z: number;
  I: number;
  K: number;
  D: number;
  A: number;
  F: number;
}

const presets = {
  metric: [
    { name: "M10 x 1.5", X: 8.5, Z: -25, I: 0, K: 1.5, D: 0.3, A: 60, F: 1.5 },
    { name: "M12 x 1.75", X: 10.25, Z: -30, I: 0, K: 1.75, D: 0.35, A: 60, F: 1.75 },
    { name: "M16 x 2.0", X: 14, Z: -35, I: 0, K: 2.0, D: 0.4, A: 60, F: 2.0 },
    { name: "M20 x 2.5", X: 17.5, Z: -40, I: 0, K: 2.5, D: 0.5, A: 60, F: 2.5 },
    { name: "M24 x 3.0", X: 21, Z: -45, I: 0, K: 3.0, D: 0.6, A: 60, F: 3.0 }
  ],
  imperial: [
    { name: "1/4-20 UNC", X: 5.537, Z: -25.4, I: 0, K: 1.27, D: 0.254, A: 60, F: 1.27 },
    { name: "5/16-18 UNC", X: 6.985, Z: -30.48, I: 0, K: 1.411, D: 0.282, A: 60, F: 1.411 },
    { name: "3/8-16 UNC", X: 8.432, Z: -35.56, I: 0, K: 1.588, D: 0.318, A: 60, F: 1.588 },
    { name: "1/2-13 UNC", X: 11.074, Z: -40.64, I: 0, K: 1.954, D: 0.391, A: 60, F: 1.954 },
    { name: "5/8-11 UNC", X: 14.072, Z: -45.72, I: 0, K: 2.309, D: 0.462, A: 60, F: 2.309 }
  ],
  special: [
    { name: "Acme 1/2-10", X: 10.583, Z: -30, I: 0, K: 2.54, D: 0.508, A: 29, F: 2.54 },
    { name: "Acme 5/8-8", X: 13.096, Z: -35, I: 0, K: 3.175, D: 0.635, A: 29, F: 3.175 },
    { name: "Pipe 1/8-27 NPT", X: 8.611, Z: -20, I: 0.625, K: 0.941, D: 0.188, A: 60, F: 0.941 },
    { name: "Pipe 1/4-18 NPT", X: 11.445, Z: -25, I: 0.625, K: 1.411, D: 0.282, A: 60, F: 1.411 },
    { name: "Trapezoidal 30x6", X: 24, Z: -40, I: 0, K: 6, D: 1.2, A: 30, F: 6 }
  ]
};

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