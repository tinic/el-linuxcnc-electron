<script setup lang="ts">
import { ref, onMounted, inject } from "vue";
import { useDialog } from "primevue/usedialog";

import json from '../assets/presets.json';
import { showCompletionScript } from "yargs";
import { emit } from "process";

const emit = defineEmits(['selected']);
const dialogRef = inject("dialogRef");
const dialog = useDialog();

var headers:string[] = [];

var axis:number = 0;
for (var i:number = 0; i < json.length; i++) {
    if (json[i].axis == dialogRef.value.data.axis) {
        axis = i;
        break;
    }
}
for (var i:number = 0; i < json[axis].sections.length; i++) {
    headers.push(json[axis].sections[i].header);
}

const selectPitch = (name:string, pitch:number) => {
    emit('selected',dialogRef.value.data.axis,name,pitch);
    dialogRef.value.close();
};

onMounted(() => {
});

</script>

<template>  
    <TabView>
        <TabPanel v-for="(title, sindex) in headers" :header="title">
            <div class="grid dro-font-preset-button">
                <div class="col-3 p-1" v-for="(pitch, pindex) in json[axis].sections[sindex].pitches">
                    <button @click="selectPitch(pitch.name,pitch.pitch)" class="w-full h-full">{{ pitch.name }}</button>
                    <br/>
                </div>            
            </div>
        </TabPanel>
    </TabView>
</template>

<style>

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
 