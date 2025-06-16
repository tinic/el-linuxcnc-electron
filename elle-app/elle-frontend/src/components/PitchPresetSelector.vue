<script setup lang="ts">
import { onMounted, inject } from 'vue'
import { useDialog } from 'primevue/usedialog'

import json from '../assets/manualpresets.json'

const emit = defineEmits(['selected'])
const dialogRef = inject('dialogRef') as any
// eslint-disable-next-line no-unused-vars
const dialog = useDialog()

const headers: string[] = []

let axis: number = 0
for (var i: number = 0; i < json.length; i++) {
  if (json[i].axis == dialogRef.value.data.axis) {
    axis = i
    break
  }
}
for (var i: number = 0; i < json[axis].sections.length; i++) {
  headers.push(json[axis].sections[i].header)
}

const selectPitch = (name: string, pitch: number, type: string) => {
  emit('selected', dialogRef.value.data.axis, name, pitch, type)
  dialogRef.value.close()
}

onMounted(() => {})
</script>

<template>
  <TabView>
    <TabPanel
      v-for="(title, sindex) in headers"
      :key="sindex"
      :value="sindex"
      :header="title"
    >
      <div class="grid dro-font-preset-button">
        <div v-for="(pitch, pindex) in json[axis].sections[sindex].pitches" class="col-3 p-1">
          <button
            class="w-full h-full button-pitchselector"
            @click="selectPitch(pitch.name, pitch.value, pitch.type)"
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
  font-family: 'iosevka';
  font-weight: bold;
  font-size: 1.75em;
  text-align: center;
}
</style>
