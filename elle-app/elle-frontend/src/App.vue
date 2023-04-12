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
const xpitchactive = ref(false);
const zpitchactive = ref(false);
const xstepperactive = ref(false);
const zstepperactive = ref(false);

// Internal
const numberentry = ref(0);
const xpitchlabel = ref('…');
const zpitchlabel = ref('…');
const xpitchangle = ref(0);

let zforward:boolean = true;
let xforward:boolean = true;
let xaxisoffset:number = 0;
let zaxisoffset:number = 0;
let aaxisoffset:number = 0;
let xaxisset:number = 0;
let zaxisset:number = 0;
let aaxisset:number = 0;
let xaxissetscheduled:boolean = false;
let zaxissetscheduled:boolean = false;
let aaxissetscheduled:boolean = false;

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
  window.api.send('startHAL');
}

const stopHAL = () => {
  window.api.send('stopHAL');
}

const clearOutput = () => {
  halStdoutText.value = '';
}

const quitApplication = () => {
  window.api.send('quit');
};

let halOutScheduled:boolean = false;
let updateInterval:NodeJS.Timer;

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
        if (zaxissetscheduled) {
          zaxissetscheduled = false;
          zaxisoffset = (halIn as any).position_z + zaxisset;
          zaxisset = 0;
        }
        if (xaxissetscheduled) {
          xaxissetscheduled = false;
          xaxisoffset = (-(halIn as any).position_x) + xaxisset;
          xaxisset = 0;
        }
        if (aaxissetscheduled) {
          aaxissetscheduled = false;
          aaxisoffset = (halIn as any).position_a + aaxisset;
          aaxisset = 0;
        }
        zpos.value = (halIn as any).position_z - zaxisoffset;
        xpos.value = (-(halIn as any).position_x) - xaxisoffset;
        apos.value = Math.abs((((halIn as any).position_a - aaxisoffset) % 1) * 360);
        rpms.value = Math.abs((halIn as any).speed_rps * 60);
      });
    } catch {
      // nop
    }
    if (halOutScheduled) {
      halOutScheduled = false;
      let halOut = {
          "forward_z" : zforward ? -zpitch.value : zpitch.value,
          "forward_x" : xforward ? -xpitch.value : xpitch.value,
          "enable_z" : zpitchactive.value,
          "enable_x" : xpitchactive.value,
          "enable_stepper_z" : zstepperactive.value,
          "enable_stepper_x" : xstepperactive.value
      };
      putHalOut(halOut);
    }
  }, 33.33333);
}

function endPoll() {
  clearTimeout(updateInterval);
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
        zstepperactive.value = true;
        xstepperactive.value = false;
        zpitchactive.value = true;
        xpitchactive.value = false;
        zforward = true;
        xforward = true;
      break;
      case DirectionMode.reverse:
        zstepperactive.value = true;
        xstepperactive.value = false;
        zpitchactive.value = true;
        xpitchactive.value = false;
        zforward = false;
        xforward = false;
      break;
      case DirectionMode.hold:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = true;
        xpitchactive.value = false;
        zforward = true;
        xforward = true;
      break;
      case DirectionMode.idle:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = false;
        xpitchactive.value = false;
        zforward = true;
        xforward = true;
      break;
    }
    break;
    case FeedMode.cross:
    switch(selectedDirectionMode.value) {
      case DirectionMode.forward:
        zstepperactive.value = false;
        xstepperactive.value = true;
        zpitchactive.value = false;
        xpitchactive.value = true;
        zforward = true;
        xforward = false;
      break;
      case DirectionMode.reverse:
        zstepperactive.value = false;
        xstepperactive.value = true;
        zpitchactive.value = false;
        xpitchactive.value = true;
        zforward = false;
        xforward = true;
      break;
      case DirectionMode.hold:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = false;
        xpitchactive.value = true;
        zforward = true;
        xforward = true;
      break;
      case DirectionMode.idle:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = false;
        xpitchactive.value = false;
        zforward = true;
        xforward = true;
      break;
    }
    break;
    case FeedMode.frontCompound:
    switch(selectedDirectionMode.value) {
      case DirectionMode.forward:
        zstepperactive.value = true;
        xstepperactive.value = true;
        zpitchactive.value = true;
        xpitchactive.value = true;
        zforward = true;
        xforward = true;
      break;
      case DirectionMode.reverse:
        zstepperactive.value = true;
        xstepperactive.value = true;
        zpitchactive.value = true;
        xpitchactive.value = true;
        zforward = false;
        xforward = false;
      break;
      case DirectionMode.hold:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = true;
        xpitchactive.value = true;
        zforward = true;
        xforward = true;
      break;
      case DirectionMode.idle:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = false;
        xpitchactive.value = false;
        zforward = true;
        xforward = true;
      break;
    }
    break;
    case FeedMode.backCompound:
    switch(selectedDirectionMode.value) {
      case DirectionMode.forward:
        zstepperactive.value = true;
        xstepperactive.value = true;
        zpitchactive.value = true;
        xpitchactive.value = true;
        zforward = true;
        xforward = false;
      break;
      case DirectionMode.reverse:
        zstepperactive.value = true;
        xstepperactive.value = true;
        zpitchactive.value = true;
        xpitchactive.value = true;
        zforward = false;
        xforward = true;
      break;
      case DirectionMode.hold:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = true;
        xpitchactive.value = true;
        zforward = true;
        xforward = false;
      break;
      case DirectionMode.idle:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = false;
        xpitchactive.value = false;
        zforward = true;
        xforward = false;
      break;
    }
    break;
  }
  scheduleHALOut();
}

const zeroClicked = (arg:number) => {
  switch(arg) {
      case 1:
      xaxisset = 0;
      xaxissetscheduled = true;
      scheduleHALOut();
      break;
      case 2:
      zaxisset = 0;
      zaxissetscheduled = true;
      scheduleHALOut();
      break;
      case 3:
      aaxisset = 0;
      aaxissetscheduled = true;
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

window.api.receive('halStarted', () => {
  console.log("receive:halStarted");
  selectedMenu.value = 0;
  startPoll();
  updateHALOut();
});

window.api.receive('halStopped', () => {
  console.log("receive:halStopped");
  endPoll();
});

window.api.receive('halStdout', (event:any, arg:any) => {
  console.log("receive:halStdout");
  halStdoutText.value += event as string;
});

selectedMenu.value = 2;
startHAL();

</script>

<script lang="ts">

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
          :xlock="xpitchactive"
          :zlock="zpitchactive"
          :xpitchactive="xstepperactive"
          :zpitchactive="zstepperactive"
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
            <Button @click="clearOutput" label="Clear Output" />
          </template>
        </Toolbar>
        <Textarea v-model="halStdoutText" autoScroll="true" rows="30" cols="30" />
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
