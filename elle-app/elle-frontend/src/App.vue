<script setup lang="ts">
import { markRaw, ref, defineAsyncComponent } from 'vue';
import { useDialog } from 'primevue/usedialog';

import DRODisplay from './components/DRODisplay.vue';
import Numpad from './components/Numpad.vue';

const PitchSelector = defineAsyncComponent(() => import('./components/PitchSelector.vue'));

const selectedMenu = ref(0);

const xpos = ref(0);
const zpos = ref(0);
const apos = ref(0);
const rpms = ref(5000);
const xpitch = ref(0.3);
const zpitch = ref(0.3);
const xlock = ref(false);
const zlock = ref(true);
const xpitchactive = ref(false);
const zpitchactive = ref(true);
const numberentry = ref(0);
const xpitchlabel = ref('…');
const zpitchlabel = ref('…');

const menuItems = ref([
    { separator: true },
    { label: 'Home', 
      icon: 'pi pi-fw pi-home',
      command: () => {
        selectedMenu.value = 0;
      }
    },
    { label: 'CC', 
      icon: 'pi pi-fw pi-link',
      command: () => {
        selectedMenu.value = 1;
      }
    },
    { label: 'HAL', 
      icon: 'pi pi-fw pi-link',
      command: () => {
        selectedMenu.value = 2;
      }
    },
    { label: 'Settings', 
      icon: 'pi pi-fw pi-cog',
      command: () => {
        selectedMenu.value = 3;
      }
    },
    { separator: true }
]); 

setInterval(() => {
  xpos.value += 1.0;
  zpos.value -= 0.1;
  apos.value -= 33.3;
  }, 33.333333);

const numberClicked = (arg:number) => {
  console.log("numberClicked" + arg);
};

const zeroClicked = (arg:number) => {
  switch(arg) {
      case 1:
      xpos.value = 0;
      break;
      case 2:
      zpos.value = 0;
      break;
      case 3:
      apos.value = 0;
      break;
  }
};

const dialog = useDialog();

const pitchClicked = (axis:string) => {
  const dialogRef = dialog.open(PitchSelector, {
        props: {
            header: 'Select Pitch',
            style: {
                width: '70vw',
            },
            breakpoints:{
                '960px': '75vw',
                '640px': '90vw'
            },
            position: 'top',
            modal: true,
        },
        data: {
          axis: axis
        },
        emits: {
          onSelected: (axis:string, name:string, pitch:number, type:string) => {
            console.log('sdfsd')
            switch(axis) {
              case 'z':
                zpitch.value = pitch;
                zpitchlabel.value = name;
                break;
              case 'x':
                xpitch.value = pitch;
                xpitchlabel.value = name;
                break;
            }
          }
        },
        templates: {
        },
        onClose: (options) => {
        }
    });
};

</script>

<template>
  <div class="flex flex-row flex-grow-1 absolute top-0 left-0 wrapper bg-gray-800">
    <Menu v-model="selectedMenu" :model="menuItems" class="flex-none">
      <template #start>
        <button class="w-full p-link flex align-items-center p-2 pl-3 text-color hover:surface-200 border-noround">
          <div class="flex flex-column align">
          <span class="font-bold">Elle</span>
          </div>
        </button>
      </template>
      <template #end>
        <button class="w-full p-link bottom-0flex align-items-center p-2 pl-4 text-color hover:surface-200 border-noround ">
          <i class="pi pi-sign-out" />
          <span class="ml-2">Exit</span>
        </button>
      </template>
    </Menu>
    <div v-if="selectedMenu==0" class="m-5">
      <div class="flex flex-row">
        <DRODisplay class="mr-2 h-min"
          :xpos="xpos"
          :zpos="zpos"
          :apos="apos"
          :rpms="rpms"
          :xpitch="xpitch"
          :zpitch="zpitch"
          :xlock="xlock"
          :zlock="zlock"
          :xpitchactive="xpitchactive"
          :zpitchactive="zpitchactive"
          :numberentry="numberentry"
          :xpitchlabel="xpitchlabel"
          :zpitchlabel="zpitchlabel"
          @numberClicked="numberClicked"
          @zeroClicked="zeroClicked"
          @pitchClicked="pitchClicked"/>
          <Numpad class=""/>
          <DynamicDialog/>
      </div>
    </div>
    <div v-if="selectedMenu==1" class="flex-grow-1 flex align-items-center justify-content-center bg-blue-500 ">
      Canned cycles
    </div>
    <div v-if="selectedMenu==2" class="flex-grow-1 flex align-items-center m-3 justify-content-center ">
      <div class="flex flex-column w-full h-full">
        <Toolbar class="mb-2">
          <template #start>
              <Button label="Start HAL" icon="pi pi-play" class="mr-2" />
              <Button label="Stop HAL" icon="pi pi-stop" severity="success" />
          </template>
          <template #end>
            <Button label="Clear Output" class="" />
          </template>
        </Toolbar>
        <ScrollPanel class="bg-gray-900 p-2 h-full text-left fixed-width-font">
          "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."\
        </ScrollPanel>
      </div>
    </div>
    <div v-if="selectedMenu==3" class="flex-grow-1 flex align-items-center justify-content-center bg-blue-500 ">
      Settings
    </div>
  </div>
</template>

<style scoped>

.fixed-width-font {
  font-family: 'iosevka';
  font-weight: normal;
}

.wrapper, html, body {
    width: 100%;
    height: 100%;
    margin: 0;
    color: #ffffff;
    background-color: #222222;
}

.wrapper {
    display: flex;
    flex-direction: column;
}

</style>
