<script setup lang="ts">
import { onMounted, inject } from "vue";
import { useDialog } from "primevue/usedialog";

import json from "../assets/manualpresets.json";

import DialogRef from "primevue/dialog";

const emit = defineEmits(["selected"]);
const dialogRef = inject("dialogRef") as any;
const dialog = useDialog();

var headers: string[] = [];

var axis: number = 0;
for (var i: number = 0; i < json.length; i++) {
  if (json[i].axis == dialogRef.value.data.axis) {
    axis = i;
    break;
  }
}
for (var i: number = 0; i < json[axis].sections.length; i++) {
  headers.push(json[axis].sections[i].header);
}

const selectPitch = (name: string, pitch: number, type: string) => {
  emit("selected", dialogRef.value.data.axis, name, pitch, type);
  dialogRef.value.close();
};

onMounted(() => {});
</script>

<template>
  <TabView>
    <TabPanel v-for="(title, sindex) in headers" :key="sindex" :value="sindex" :header="title">
      <div class="grid dro-font-preset-button">
        <div
          class="col-3 p-1"
          v-for="(pitch, pindex) in json[axis].sections[sindex].pitches"
        >
          <button
            @click="selectPitch(pitch.name, pitch.value, pitch.type)"
            class="w-full h-full button-pitchselector"
          >
            {{ pitch.name }}
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
