<script setup lang="ts">
import { ref, computed, watch, defineAsyncComponent } from 'vue';
import { useDialog } from 'primevue/usedialog';

import Numpad from './components/Numpad.vue';
import DRODisplay from './components/DRODisplay.vue';

const halOutURL = 'http://localhost:8000/hal/hal_out';
const halInURL = 'http://localhost:8000/hal/hal_in';

const selectedMenu = ref(0);

// Polled over REST
const xpos = ref(0);
const zpos = ref(0);
const apos = ref(0);
const rpms = ref(0);

// Pushed over REST
const xpitch = ref(0.1);
const zpitch = ref(0.1);
const xlock = ref(false);
const zlock = ref(false);
const xpitchactive = ref(false);
const zpitchactive = ref(false);
const zforward = ref(true)
const xforward = ref(true)

// Internal
const numberentry = ref(0);
const xpitchlabel = ref('…');
const zpitchlabel = ref('…');
const xpitchangle = ref(0);
const zaxiszero = ref(0)
const xaxiszero = ref(0)
const aaxiszero = ref(0)

enum FeedMode {
  none=0,
  longitudinal=1,
  cross=2,
  frontCompound=3,
  backCompound=4,
}
const selectedFeedMode = ref(FeedMode.longitudinal);

enum DirectionMode {
  none=0,
  forward=1,
  reverse=2,
  hold=3,
  idle=4
}
const selectedDirectionMode = ref(DirectionMode.forward);

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

const numberClicked = (arg:number) => {
  console.log("numberClicked" + arg);
};

const halStdoutText = ref('');

const startHAL = () => {
  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(' electron/') > -1) {
    const { ipcRenderer } = window.require('electron');
    ipcRenderer.send('startHAL');
  }
}

const stopHAL = () => {
  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(' electron/') > -1) {
    const { ipcRenderer } = window.require('electron');
    ipcRenderer.send('stopHAL');
  }
}

const quitApplication = () => {
  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(' electron/') > -1) {
    const { ipcRenderer } = window.require('electron');
    ipcRenderer.send('quit');
  }
};

var halOutScheduled:boolean = false;
var updateInterval:NodeJS.Timer;

interface HalIn {
  position_z:number,
  position_x:number,
  position_a:number,
  speed_rps:number
};

async function putHalOut(halOut:Object) {
  try {
    const response = await fetch(halOutURL, {
      method: "PUT",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(halOut),
    });
    const result = await response.json();
    console.log("Success:", result);
  } catch (error) {
    console.error("Error:", error);
  }
}

function getHalIn(): Promise<HalIn[]> {
  return fetch(halInURL)
                .then(res => res.json())
                .then(res => {
                  return res as HalIn[]
                })
};

function startPoll() {
  updateInterval = setInterval(() => {
    try {
      getHalIn().then(halIn => {
        zpos.value = (halIn as any).position_z - zaxiszero.value;
        xpos.value = (halIn as any).position_x - xaxiszero.value;
        apos.value = Math.abs((((halIn as any).position_a - aaxiszero.value) % 1) * 360);
        rpms.value = Math.abs((halIn as any).speed_rps * 60);
      });
    } catch {
      // nop
    }
    if (halOutScheduled) {
      halOutScheduled = false;
      let halOut = {
          "forward_z" : zforward.value ? -zpitch.value : zpitch.value,
          "forward_x" : xforward.value ? -xpitch.value : xpitch.value,
          "enable_z" : zpitchactive.value,
          "enable_x" : xpitchactive.value,
          "enable_stepper_z" : zlock.value,
          "enable_stepper_x" : xlock.value
      };
      putHalOut(halOut);
    }
  }, 66);
}

function endPoll() {
  clearTimeout(updateInterval);
}

var userAgent = navigator.userAgent.toLowerCase();
if (userAgent.indexOf(' electron/') > -1) {
  const { ipcRenderer } = window.require('electron');
  ipcRenderer.on('halStarted', () => {
    startPoll();
  });

  ipcRenderer.on('halStopped', () => {
    endPoll();
  });

  ipcRenderer.on('halStdout', (event:any, arg:any) => {
      halStdoutText.value += arg as string;
      event.returnValue = true;
  });
} else {
  startPoll();
}

const forwardIcon = computed(() => {
  switch (selectedFeedMode.value) {
    case FeedMode.longitudinal:
    return '⬅'
    case FeedMode.cross:
    return '⬆'
    case FeedMode.frontCompound:
    return '⬋'
    case FeedMode.backCompound:
    return '⬉'
  }
});

const reverseIcon = computed(() => {
  switch (selectedFeedMode.value) {
    case FeedMode.longitudinal:
    return '⮕'
    case FeedMode.cross:
    return '⬇'
    case FeedMode.frontCompound:
    return '⬈'
    case FeedMode.backCompound:
    return '⬊'
  }
});

const feedModeLongitudinalClicked = () => {
  selectedFeedMode.value = FeedMode.longitudinal;
}
const feedModeCrossClicked = () => {
  selectedFeedMode.value = FeedMode.cross;
}
const feedModeFrontCompoundClicked = () => {
  selectedFeedMode.value = FeedMode.frontCompound;
}

const feedModeBackCompoundClicked = () => {
  selectedFeedMode.value = FeedMode.backCompound;
}

const directionModeForwardClicked = () => {
  selectedDirectionMode.value = DirectionMode.forward;
}
const directionModeReverseClicked = () => {
  selectedDirectionMode.value = DirectionMode.reverse;
}
const directionModeHoldClicked = () => {
  selectedDirectionMode.value = DirectionMode.hold;
}
const directionModeIdleClicked = () => {
  selectedDirectionMode.value = DirectionMode.idle;
}

function scheduleHALOut() {
  halOutScheduled = true;
}

function updateHALOut() {
  switch(selectedFeedMode.value) {
    case FeedMode.longitudinal:
    switch(selectedDirectionMode.value) {
      case DirectionMode.forward:
        zpitchactive.value = true;
        xpitchactive.value = false;
        zlock.value = true;
        xlock.value = false;
        zforward.value = true;
        xforward.value = true;
      break;
      case DirectionMode.reverse:
        zpitchactive.value = true;
        xpitchactive.value = false;
        zlock.value = true;
        xlock.value = false;
        zforward.value = false;
        xforward.value = false;
      break;
      case DirectionMode.hold:
        zpitchactive.value = false;
        xpitchactive.value = false;
        zlock.value = true;
        xlock.value = false;
        zforward.value = true;
        xforward.value = true;
      break;
      case DirectionMode.idle:
        zpitchactive.value = false;
        xpitchactive.value = false;
        zlock.value = false;
        xlock.value = false;
        zforward.value = true;
        xforward.value = true;
      break;
    }
    break;
    case FeedMode.cross:
    switch(selectedDirectionMode.value) {
      case DirectionMode.forward:
        zpitchactive.value = false;
        xpitchactive.value = true;
        zlock.value = false;
        xlock.value = true;
        zforward.value = true;
        xforward.value = true;
      break;
      case DirectionMode.reverse:
        zpitchactive.value = false;
        xpitchactive.value = true;
        zlock.value = false;
        xlock.value = true;
        zforward.value = false;
        xforward.value = false;
      break;
      case DirectionMode.hold:
        zpitchactive.value = false;
        xpitchactive.value = false;
        zlock.value = false;
        xlock.value = true;
        zforward.value = true;
        xforward.value = true;
      break;
      case DirectionMode.idle:
        zpitchactive.value = false;
        xpitchactive.value = false;
        zlock.value = false;
        xlock.value = false;
        zforward.value = true;
        xforward.value = true;
      break;
    }
    break;
    case FeedMode.backCompound:
    case FeedMode.frontCompound:
    switch(selectedDirectionMode.value) {
      case DirectionMode.forward:
        zpitchactive.value = true;
        xpitchactive.value = true;
        zlock.value = true;
        xlock.value = true;
        zforward.value = true;
        xforward.value = true;
      break;
      case DirectionMode.reverse:
        zpitchactive.value = true;
        xpitchactive.value = true;
        zlock.value = true;
        xlock.value = true;
        zforward.value = false;
        xforward.value = false;
      break;
      case DirectionMode.hold:
        zpitchactive.value = false;
        xpitchactive.value = false;
        zlock.value = true;
        xlock.value = true;
        zforward.value = true;
        xforward.value = true;
      break;
      case DirectionMode.idle:
        zpitchactive.value = false;
        xpitchactive.value = false;
        zlock.value = false;
        xlock.value = false;
        zforward.value = false;
        xforward.value = false;
      break;
    }
    break;
  }
  scheduleHALOut();
}

const zeroClicked = (arg:number) => {
  switch(arg) {
      case 1:
      xaxiszero.value = xpos.value;
      scheduleHALOut();
      break;
      case 2:
      zaxiszero.value = zpos.value;
      scheduleHALOut();
      break;
      case 3:
      aaxiszero.value = apos.value;
      scheduleHALOut();
      break;
  }
};

watch([selectedFeedMode, selectedDirectionMode], () => {
  updateHALOut();
});

watch([zpitch, xpitch], () => {
  updateHALOut();
})

const PitchSelector = defineAsyncComponent(() => import('./components/PitchSelector.vue'));

function pitchForAngle(pitch:number, angle:number) {
  return pitch * Math.tan(angle * (Math.PI / 180))
}

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
          onSelected: (axis:string, name:string, value:number, type:string) => {
            switch(axis) {
              case 'z':
                if (type != 'angle') {
                  zpitch.value = value;
                  zpitchlabel.value = name;
                  if (xpitchangle.value > 0) {
                    xpitch.value = pitchForAngle(zpitch.value, xpitchangle.value);
                  }
                }
                break;
              case 'x':
                if (type != 'angle') {
                  xpitch.value = value;
                  xpitchlabel.value = name;
                  xpitchangle.value = 0;
                } else {
                  xpitch.value = pitchForAngle(zpitch.value, value);
                  xpitchlabel.value = name;
                  xpitchangle.value = value;
                }
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

updateHALOut();

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
        <button @click="quitApplication" class="w-full p-link bottom-0flex align-items-center p-2 pl-4 text-color hover:surface-200 border-noround ">
          <i class="pi pi-sign-out" />
          <span class="ml-2">Exit</span>
        </button>
      </template>
    </Menu>
    <div v-if="selectedMenu==0" class="m-2">
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
        </div>
        <div class="flex flex-row">
          <div class="grid dro-font-mode grid-nogutter p-3 pr-4 m-0 mt-2 bg-gray-900" style="width:16em">
            <div class="col-12 align-content-center">Feed</div>
            <button @click="feedModeLongitudinalClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedFeedMode == FeedMode.longitudinal" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                ⬌ Longitudinal
              </span>
            </button>
            <button @click="feedModeCrossClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedFeedMode == FeedMode.cross" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                ⬍ Cross
              </span>
            </button>
            <button @click="feedModeFrontCompoundClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedFeedMode == FeedMode.frontCompound" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                ⬋ Front Compound
              </span>
            </button>
            <button @click="feedModeBackCompoundClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedFeedMode == FeedMode.backCompound" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                ⬉ Back Compound
              </span>
            </button>
          </div>
          <div class="grid dro-font-mode grid-nogutter p-3 pr-4 m-0 mt-2 bg-gray-900" style="width:15em">
            <div class="col-12 align-content-center">Direction</div>
            <button @click="directionModeForwardClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedDirectionMode == DirectionMode.forward" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                {{ forwardIcon }} Forward
              </span>
            </button>
            <button @click="directionModeReverseClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedDirectionMode == DirectionMode.reverse" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                {{ reverseIcon }} Reverse
              </span>
            </button>
            <button @click="directionModeHoldClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedDirectionMode == DirectionMode.hold" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                ⏸ Hold
              </span>
            </button>
            <button @click="directionModeIdleClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedDirectionMode == DirectionMode.idle" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                ⏹ Idle
              </span>
            </button>
          </div>
          <div class="grid grid-nogutter bg-gray-900 mt-2 p-1 dro-font-mode" style="width:24em">
              <div class="col-4 p-1"></div>
              <div class="col-4 p-1"><button class="button-mode button-direction w-full h-full">⏶</button></div>
              <div class="col-4 p-1"></div>
              <div class="col-4 p-1"><button class="button-mode button-direction w-full h-full">⏴</button></div>
              <div class="col-4 p-1"><button class="button-mode button-direction w-full h-full">STOP</button></div>
              <div class="col-4 p-1"><button class="button-mode button-direction w-full h-full">⏵</button></div>
              <div class="col-4 p-1"></div>
              <div class="col-4 p-1"><button class="button-mode button-direction w-full h-full">⏷</button></div>
              <div class="col-4 p-1"></div>
            </div>
        </div>
        <DynamicDialog/>
    </div>
    <div v-if="selectedMenu==1" class="flex-grow-1 flex align-items-center justify-content-center bg-blue-500 ">
      Canned cycles
    </div>
    <div v-if="selectedMenu==2" class="flex-grow-1 flex align-items-center m-2 justify-content-center ">
      <div class="flex flex-column w-full h-full">
        <Toolbar class="mb-2 bg-gray-900">
          <template #start>
              <Button @click="startHAL" label="Start HAL" icon="pi pi-play" class="mr-2" />
              <Button @click="stopHAL" label="Stop HAL" icon="pi pi-stop" severity="success" />
          </template>
          <template #end>
            <Button label="Clear Output" class="" />
          </template>
        </Toolbar>
        <ScrollPanel class="bg-gray-900 p-2 h-full text-left fixed-width-font">
          {{ halStdoutText }}
        </ScrollPanel>
      </div>
    </div>
    <div v-if="selectedMenu==3" class="flex-grow-1 flex align-items-center justify-content-center bg-blue-500 ">
      Settings
    </div>
  </div>
</template>

<style scoped>
.button-mode {
    background: #333;
    color: #ffffff;
}

.button-direction {
  font-size: 1.5em;
}

.dro-font-mode {
  font-family: 'iosevka';
  font-weight: bold;
  font-size: 1.1em;
  text-align: center;
}

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
