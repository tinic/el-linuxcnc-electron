<script setup lang="ts">
import { ref, onMounted, computed, watch, defineAsyncComponent } from "vue";
import { useDialog } from "primevue/usedialog";
import { Camera, Renderer, RendererPublicInterface, Scene } from "troisjs";

import Numpad from "./components/Numpad.vue";
import DRODisplay from "./components/DRODisplay.vue";
import Backplot from "./Backplot";
import { putHalOut, putLinuxCNC, getHalIn } from "./HAL"

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
const xpitchlabel = ref("…");
const zpitchlabel = ref("…");
const xpitchangle = ref(0);
const metric = ref(true);
const cursorpos = ref(0);

let zforward: boolean = true;
let xforward: boolean = true;
let xaxisoffset: number = 0;
let zaxisoffset: number = 0;
let aaxisoffset: number = 0;
let xaxisset: number = 0;
let zaxisset: number = 0;
let aaxisset: number = 0;
let xaxissetscheduled: boolean = false;
let zaxissetscheduled: boolean = false;
let aaxissetscheduled: boolean = false;

let buttonuptime: number = 0;
let buttondowntime: number = 0;
let buttonlefttime: number = 0;
let buttonrighttime: number = 0;
let buttonupscheduled: boolean = false;

enum FeedMode {
  none = 0,
  longitudinal = 1,
  cross = 2,
  frontCompound = 3,
  backCompound = 4,
}
const selectedFeedMode = ref(FeedMode.longitudinal);

enum DirectionMode {
  none = 0,
  forward = 1,
  reverse = 2,
  hold = 3,
  idle = 4,
}
const selectedDirectionMode = ref(DirectionMode.forward);

const menuItems = ref([
  { separator: true },
  {
    label: "Home",
    icon: "pi pi-fw pi-home",
    command: () => {
      selectedMenu.value = 0;
    },
  },
  {
    label: "CC",
    icon: "pi pi-fw pi-link",
    command: () => {
      selectedMenu.value = 1;
    },
  },
  {
    label: "HAL",
    icon: "pi pi-fw pi-link",
    command: () => {
      selectedMenu.value = 2;
    },
  },
  {
    label: "Settings",
    icon: "pi pi-fw pi-cog",
    command: () => {
      selectedMenu.value = 3;
    },
  },
  { separator: true },
]);

enum NumpadInputStage {
  none = 0,
  start = 1,
  entry = 2,
}

const entryActive = ref(0);

let numpadInputStage = NumpadInputStage.none;
let numbersClicked = new Array<string>();
let numbersNegative = false;
let numbersPrevious: number = 0;

function treatOffClickAsEnter() {
  if (numpadInputStage == NumpadInputStage.start) {
    numberentry.value = numbersPrevious;
    setFinalNumber(numbersPrevious);
  } else if (numpadInputStage == NumpadInputStage.entry) {
    numberentry.value = calcNumber();
    setFinalNumber(numberentry.value);
  }
}

const numberClicked = (entry: number, value: number) => {
  treatOffClickAsEnter();
  numbersClicked.length = 0;
  numpadInputStage = NumpadInputStage.start;
  switch (entry) {
    case 1:
    case 2:
    case 4:
    case 5:
    numberentry.value = numbersPrevious = metric.value ? value : value / 25.4;
    break;
    case 3:
    numberentry.value = numbersPrevious = value;
    break;
  }
  entryActive.value = entry;
  numbersNegative = false;
};

function calcNumber(): number {
  const dotIndex = numbersClicked.indexOf(".");
  let integerSize = 0;
  let fractionSize = 0;
  if (dotIndex >= 0) {
    integerSize = dotIndex;
    fractionSize = numbersClicked.length - dotIndex - 1;
    cursorpos.value = numbersClicked.length - dotIndex + 1;
  } else {
    integerSize = numbersClicked.length;
    if (numbersClicked.length == 0) {
      cursorpos.value = 0;
    } else {
      cursorpos.value = 1;
    }
  }
  let value: number = 0;
  for (let i = 0; i < integerSize; i++) {
    value +=
      (numbersClicked[i].charCodeAt(0) - 0x30) *
      Math.pow(10, integerSize - i - 1);
  }
  for (let i = 0; i < fractionSize; i++) {
    value +=
      (numbersClicked[i + integerSize + 1].charCodeAt(0) - 0x30) *
      Math.pow(10, -i - 1);
  }
  return value * (numbersNegative ? -1 : +1);
}

function setFinalNumber(value: number) {
  switch (entryActive.value) {
    case 1:
    case 2:
    case 4:
    case 5:
    if (!metric.value) {
      value = value * 25.4;
    }
    break;
    case 3:
    //nop
    break;
  }
  switch (entryActive.value) {
    case 1:
      xaxisset = value;
      xaxissetscheduled = true;
      xpos.value = value;
      break;
    case 2:
      zaxisset = value;
      zaxissetscheduled = true;
      zpos.value = value;
      break;
    case 3:
      aaxisset = value;
      aaxissetscheduled = true;
      apos.value = value;
      break;
    case 4:
      xpitch.value = Math.abs(value);
      break;
    case 5:
      zpitch.value = Math.abs(value);
      break;
  }
  numpadInputStage = NumpadInputStage.none;
  numbersClicked.length = 0;
  entryActive.value = 0;
  cursorpos.value = 0;
}

const numPadClicked = (key: string) => {
  if (numpadInputStage == NumpadInputStage.none) {
    return;
  }
  switch (key) {
    case "Escape":
      numpadInputStage = NumpadInputStage.none;
      numberentry.value = numbersPrevious;
      numbersClicked.length = 0;
      entryActive.value = 0;
      cursorpos.value = 0;
      break;
    case "Enter":
      if (numpadInputStage == NumpadInputStage.start) {
        numberentry.value = numbersPrevious;
        setFinalNumber(numbersPrevious);
      } else if (numpadInputStage == NumpadInputStage.entry) {
        numberentry.value = calcNumber();
        setFinalNumber(numberentry.value);
      }
      break;
    case "Backspace":
      numpadInputStage = NumpadInputStage.entry;
      if (numbersClicked.at(-1) == ".") {
        numbersClicked.pop();
      }
      if (numbersClicked.length <= 1) {
        cursorpos.value = 0;
      }
      numbersClicked.pop();
      numberentry.value = calcNumber();
      break;
    case "PlusMinus":
      if (numpadInputStage == NumpadInputStage.start) {
        numbersNegative = !numbersNegative;
        numberentry.value = numbersPrevious * (numbersNegative ? -1 : +1);
        setFinalNumber(numberentry.value);
      } else {
        numbersNegative = !numbersNegative;
        numberentry.value = calcNumber();
      }
      break;
    case "Third":
      if (numpadInputStage == NumpadInputStage.start) {
        numberentry.value = numberentry.value / 3;
        setFinalNumber(numberentry.value);
      }
      break;
    case "Half":
      if (numpadInputStage == NumpadInputStage.start) {
        numberentry.value = numberentry.value / 2;
        setFinalNumber(numberentry.value);
      }
      break;
    default:
      numpadInputStage = NumpadInputStage.entry;
      numbersClicked.push(key);
      numberentry.value = calcNumber();
      break;
  }
};

const zeroClicked = (entry: number) => {
  treatOffClickAsEnter();
  entryActive.value = 0;
  switch (entry) {
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

const metricClicked = () => {
  treatOffClickAsEnter();
  metric.value = !metric.value;
};

const otherClicked = () => {
  treatOffClickAsEnter();
  entryActive.value = 0;
};

const halStdoutText = ref("");

const startHAL = () => {
  halStdoutText.value = "";
  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(" electron/") > -1) {
    window.api.send("startHAL");
  }
};

const stopHAL = () => {
  halStdoutText.value = "";
  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(" electron/") > -1) {
    window.api.send("stopHAL");
  }
};

const quitApplication = () => {
  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(" electron/") > -1) {
    window.api.send("quit");
  }
};

const rendererC = ref();

const gcodeUploader = async (event: any) => {
  const file = event.files[0];
  const reader = new FileReader();
  reader.readAsText(file);
  reader.onloadend = function () {
    putLinuxCNC("backplot", { gcode: btoa(reader.result as string) }).then(
      (json) => {
        const renderer = rendererC.value as RendererPublicInterface;
        renderer.scene?.clear();

        let backplot = new Backplot(json, renderer);
        backplot.addBoundingBoxToScene();
        backplot.addBackplotToScene();

        let xlin: number = 0;
        let xinc: number = backplot.tlin / 600;
        renderer.onBeforeRender(() => {
          backplot.updateProgress(xlin);
          xlin += xinc;
          xlin %= backplot.tlin;
        });
      }
    );
  };
};

let halOutScheduled: boolean = false;
let updateInterval: NodeJS.Timer;

function stopJogNow() {
  buttonuptime = 0;
  buttondowntime = 0;
  buttonlefttime = 0;
  buttonrighttime = 0;
  let halOut = {
    control_stop_now: 1,
  };
  putHalOut(halOut);
}

function startPoll() {
  updateInterval = setInterval(() => {
    try {
      getHalIn().then((halIn) => {
        if (xaxissetscheduled) {
          xaxissetscheduled = false;
          xaxisoffset = -(halIn as any).position_x - xaxisset;
          xaxisset = 0;
        }
        if (zaxissetscheduled) {
          zaxissetscheduled = false;
          zaxisoffset = (halIn as any).position_z - zaxisset;
          zaxisset = 0;
        }
        if (aaxissetscheduled) {
          aaxissetscheduled = false;
          aaxisoffset = (halIn as any).position_a - ((aaxisset / 360) % 1);
          aaxisset = 0;
        }
        zpos.value = (halIn as any).position_z - zaxisoffset;
        xpos.value = -(halIn as any).position_x - xaxisoffset;
        apos.value = Math.abs(
          (((halIn as any).position_a - aaxisoffset) % 1) * 360
        );
        rpms.value = Math.abs((halIn as any).speed_rps * 60);
      });
    } catch {
      // nop
    }
    if (buttonuptime > 0) {
      halOutScheduled = false;
      let velocity = (Date.now() / 1000 - buttonuptime) * 3;
      velocity = Math.min(velocity, 3.0);
      let halOut = {
        control_x_type: 1,
        velocity_x_cmd: +velocity,
      };
      putHalOut(halOut);
    }
    if (buttondowntime > 0) {
      halOutScheduled = false;
      let velocity = (Date.now() / 1000 - buttondowntime) * 3;
      velocity = Math.min(velocity, 3.0);
      let halOut = {
        control_x_type: 1,
        velocity_x_cmd: -velocity,
      };
      putHalOut(halOut);
    }
    if (buttonlefttime > 0) {
      halOutScheduled = false;
      let velocity = (Date.now() / 1000 - buttonlefttime) * 3;
      velocity = Math.min(velocity, 6.0);
      let halOut = {
        control_z_type: 1,
        velocity_z_cmd: -velocity,
      };
      putHalOut(halOut);
    }
    if (buttonrighttime > 0) {
      halOutScheduled = false;
      let velocity = (Date.now() / 1000 - buttonrighttime) * 3;
      velocity = Math.min(velocity, 6.0);
      let halOut = {
        control_z_type: 1,
        velocity_z_cmd: +velocity,
      };
      putHalOut(halOut);
    }
    if (buttonupscheduled) {
      buttonupscheduled = false;
      stopJogNow();
    }
    if (halOutScheduled) {
      halOutScheduled = false;
      let halOut = {
        control_source: false,
        forward_z: zforward ? -zpitch.value : zpitch.value,
        forward_x: xforward ? -xpitch.value : xpitch.value,
        enable_z: zpitchactive.value,
        enable_x: xpitchactive.value,
        enable_stepper_z: zstepperactive.value,
        enable_stepper_x: xstepperactive.value,
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
      return "⬅";
    case FeedMode.cross:
      return "⬆";
    case FeedMode.frontCompound:
      return "⬋";
    case FeedMode.backCompound:
      return "⬉";
  }
});

const reverseIcon = computed(() => {
  switch (selectedFeedMode.value) {
    case FeedMode.longitudinal:
      return "⮕";
    case FeedMode.cross:
      return "⬇";
    case FeedMode.frontCompound:
      return "⬈";
    case FeedMode.backCompound:
      return "⬊";
  }
});

const feedModeLongitudinalClicked = () => {
  selectedFeedMode.value = FeedMode.longitudinal;
};
const feedModeCrossClicked = () => {
  selectedFeedMode.value = FeedMode.cross;
};
const feedModeFrontCompoundClicked = () => {
  selectedFeedMode.value = FeedMode.frontCompound;
};
const feedModeBackCompoundClicked = () => {
  selectedFeedMode.value = FeedMode.backCompound;
};
const directionModeForwardClicked = () => {
  selectedDirectionMode.value = DirectionMode.forward;
};
const directionModeReverseClicked = () => {
  selectedDirectionMode.value = DirectionMode.reverse;
};
const directionModeHoldClicked = () => {
  selectedDirectionMode.value = DirectionMode.hold;
};
const directionModeIdleClicked = () => {
  selectedDirectionMode.value = DirectionMode.idle;
};

const touchStartUp = () => {
  buttonuptime = Date.now() / 1000;
};

const touchEndUp = () => {
  buttonupscheduled = true;
  stopJogNow();
};

const touchStartLeft = () => {
  buttonlefttime = Date.now() / 1000;
};

const touchEndLeft = () => {
  buttonupscheduled = true;
  stopJogNow();
};

const touchStartRight = () => {
  buttonrighttime = Date.now() / 1000;
};

const touchEndRight = () => {
  stopJogNow();
  buttonupscheduled = true;
};

const touchStartDown = () => {
  buttondowntime = Date.now() / 1000;
};

const touchEndDown = () => {
  buttonupscheduled = true;
  stopJogNow();
};

const touchStop = () => {
  buttonupscheduled = true;
  stopJogNow();
};

function scheduleHALOut() {
  halOutScheduled = true;
}

function updateHALOut() {
  switch (selectedFeedMode.value) {
    case FeedMode.longitudinal:
      switch (selectedDirectionMode.value) {
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
      switch (selectedDirectionMode.value) {
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
      switch (selectedDirectionMode.value) {
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
      switch (selectedDirectionMode.value) {
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

watch([selectedFeedMode, selectedDirectionMode], () => {
  updateHALOut();
});

watch([zpitch, xpitch], () => {
  updateHALOut();
});

const PitchSelector = defineAsyncComponent(
  () => import("./components/PitchSelector.vue")
);

function pitchForAngle(pitch: number, angle: number) {
  return pitch * Math.tan(angle * (Math.PI / 180));
}

const dialog = useDialog();
const pitchClicked = (axis: string) => {
  treatOffClickAsEnter();
  entryActive.value = 0;
  const dialogRef = dialog.open(PitchSelector, {
    props: {
      header: "Select Pitch",
      style: {
        width: "70vw",
      },
      breakpoints: {
        "960px": "75vw",
        "640px": "90vw",
      },
      position: "top",
      modal: true,
    },
    data: {
      axis: axis,
    },
    emits: {
      onSelected: (axis: string, name: string, value: number, type: string) => {
        switch (axis) {
          case "z":
            if (type != "angle") {
              zpitch.value = value;
              zpitchlabel.value = name;
              if (xpitchangle.value > 0) {
                xpitch.value = pitchForAngle(zpitch.value, xpitchangle.value);
              }
            }
            break;
          case "x":
            if (type != "angle") {
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
      },
    },
    templates: {},
    onClose: (options) => {},
  });
};

onMounted(() => {
  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(" electron/") > -1) {
    window.api.receive("halStarted", () => {
      selectedMenu.value = 0;
      startPoll();
      updateHALOut();
    });

    window.api.receive("halStopped", () => {
      endPoll();
    });

    window.api.receive("halStdout", (event: any, arg: any) => {
      halStdoutText.value += event as string;
    });

    selectedMenu.value = 2;
    startHAL();
  } else {
    startPoll();
    updateHALOut();
  }
});
</script>

<script lang="ts"></script>

<template>
  <div
    class="flex flex-row flex-grow-1 absolute top-0 left-0 wrapper bg-gray-800"
  >
    <Menu v-model="selectedMenu" :model="menuItems" class="flex-none">
      <template #start>
        <button
          class="w-full p-link flex align-items-center p-2 pl-3 text-color hover:surface-200 border-noround"
        >
          <div class="flex flex-column align">
            <span class="font-bold">Elle</span>
          </div>
        </button>
      </template>
      <template #end>
        <button
          @click="quitApplication"
          class="w-full p-link bottom-0flex align-items-center p-2 pl-4 text-color hover:surface-200 border-noround"
        >
          <i class="pi pi-sign-out" />
          <span class="ml-2">Exit</span>
        </button>
      </template>
    </Menu>
    <div v-if="selectedMenu == 0" class="m-2">
      <div class="flex flex-row">
        <DRODisplay
          class="mr-2 h-min"
          :entryActive="entryActive"
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
          :metric="metric"
          :cursorpos="cursorpos"
          @numberClicked="numberClicked"
          @zeroClicked="zeroClicked"
          @pitchClicked="pitchClicked"
          @metricClicked="metricClicked"
          @otherClicked="otherClicked"
        />
        <Numpad class="" @numPadClicked="numPadClicked" />
      </div>
      <div class="flex flex-row">
        <div
          class="grid dro-font-mode grid-nogutter p-3 pr-4 m-0 mt-2 bg-gray-900"
          style="width: 16em"
        >
          <div class="col-12 align-content-center">Feed</div>
          <button
            @click="feedModeLongitudinalClicked"
            size="large"
            class="col-12 dro-font-mode button-mode p-3 m-1"
          >
            <span class="flex flex-row align-items-center">
              <i
                v-if="selectedFeedMode == FeedMode.longitudinal"
                class="pi pi-circle-fill mr-3"
                style="color: #ff0000"
              />
              <i v-else class="pi pi-circle mr-3" />
              ⬌ Longitudinal
            </span>
          </button>
          <button
            @click="feedModeCrossClicked"
            size="large"
            class="col-12 dro-font-mode button-mode p-3 m-1"
          >
            <span class="flex flex-row align-items-center">
              <i
                v-if="selectedFeedMode == FeedMode.cross"
                class="pi pi-circle-fill mr-3"
                style="color: #ff0000"
              />
              <i v-else class="pi pi-circle mr-3" />
              ⬍ Cross
            </span>
          </button>
          <button
            @click="feedModeFrontCompoundClicked"
            size="large"
            class="col-12 dro-font-mode button-mode p-3 m-1"
          >
            <span class="flex flex-row align-items-center">
              <i
                v-if="selectedFeedMode == FeedMode.frontCompound"
                class="pi pi-circle-fill mr-3"
                style="color: #ff0000"
              />
              <i v-else class="pi pi-circle mr-3" />
              ⬋ Front Compound
            </span>
          </button>
          <button
            @click="feedModeBackCompoundClicked"
            size="large"
            class="col-12 dro-font-mode button-mode p-3 m-1"
          >
            <span class="flex flex-row align-items-center">
              <i
                v-if="selectedFeedMode == FeedMode.backCompound"
                class="pi pi-circle-fill mr-3"
                style="color: #ff0000"
              />
              <i v-else class="pi pi-circle mr-3" />
              ⬉ Back Compound
            </span>
          </button>
        </div>
        <div
          class="grid dro-font-mode grid-nogutter p-3 pr-4 m-0 mt-2 bg-gray-900"
          style="width: 15em"
        >
          <div class="col-12 align-content-center">Direction</div>
          <button
            @click="directionModeForwardClicked"
            size="large"
            class="col-12 dro-font-mode button-mode p-3 m-1"
          >
            <span class="flex flex-row align-items-center">
              <i
                v-if="selectedDirectionMode == DirectionMode.forward"
                class="pi pi-circle-fill mr-3"
                style="color: #ff0000"
              />
              <i v-else class="pi pi-circle mr-3" />
              {{ forwardIcon }} Forward
            </span>
          </button>
          <button
            @click="directionModeReverseClicked"
            size="large"
            class="col-12 dro-font-mode button-mode p-3 m-1"
          >
            <span class="flex flex-row align-items-center">
              <i
                v-if="selectedDirectionMode == DirectionMode.reverse"
                class="pi pi-circle-fill mr-3"
                style="color: #ff0000"
              />
              <i v-else class="pi pi-circle mr-3" />
              {{ reverseIcon }} Reverse
            </span>
          </button>
          <button
            @click="directionModeHoldClicked"
            size="large"
            class="col-12 dro-font-mode button-mode p-3 m-1"
          >
            <span class="flex flex-row align-items-center">
              <i
                v-if="selectedDirectionMode == DirectionMode.hold"
                class="pi pi-circle-fill mr-3"
                style="color: #ff0000"
              />
              <i v-else class="pi pi-circle mr-3" />
              ⏸ Hold
            </span>
          </button>
          <button
            @click="directionModeIdleClicked"
            size="large"
            class="col-12 dro-font-mode button-mode p-3 m-1"
          >
            <span class="flex flex-row align-items-center">
              <i
                v-if="selectedDirectionMode == DirectionMode.idle"
                class="pi pi-circle-fill mr-3"
                style="color: #ff0000"
              />
              <i v-else class="pi pi-circle mr-3" />
              ⏹ Idle
            </span>
          </button>
        </div>
        <div
          class="grid grid-nogutter bg-gray-900 mt-2 p-1 dro-font-mode"
          style="width: 24em"
        >
          <div class="col-4 p-1"></div>
          <div class="col-4 p-1">
            <button
              @touchstart="touchStartUp"
              @touchend="touchEndUp"
              @touchcancel="touchEndUp"
              @touchleave="touchEndUp"
              class="button-arrow button-direction w-full h-full"
            >
              ⏶
            </button>
          </div>
          <div class="col-4 p-1"></div>
          <div class="col-4 p-1">
            <button
              @touchstart="touchStartLeft"
              @touchend="touchEndLeft"
              @touchcancel="touchEndLeft"
              @touchleave="touchEndLeft"
              class="button-arrow button-direction w-full h-full"
            >
              ⏴
            </button>
          </div>
          <div class="col-4 p-1">
            <button 
              @touchstart="touchStop"
              @touchend="touchStop"
              @touchcancel="touchStop"
              @touchleave="touchStop"
              class="button-arrow button-direction w-full h-full">
              STOP
            </button>
          </div>
          <div class="col-4 p-1">
            <button
              @touchstart="touchStartRight"
              @touchend="touchEndRight"
              @touchcancel="touchEndRight"
              @touchleave="touchEndRight"
              class="button-arrow button-direction w-full h-full"
            >
              ⏵
            </button>
          </div>
          <div class="col-4 p-1"></div>
          <div class="col-4 p-1">
            <button
              @touchstart="touchStartDown"
              @touchend="touchEndDown"
              @touchcancel="touchEndDown"
              @touchleave="touchEndDown"
              class="button-arrow button-direction w-full h-full"
            >
              ⏷
            </button>
          </div>
          <div class="col-4 p-1"></div>
        </div>
      </div>
      <DynamicDialog />
    </div>
    <div
      v-if="selectedMenu == 1"
      class="flex-grow-1 flex align-items-center justify-content-center bg-blue-500"
    >
      <div class="flex flex-column">
        <FileUpload
          mode="basic"
          name="elle[]"
          url="/api/upload"
          accept="text/plain"
          customUpload
          :auto="true"
          @uploader="gcodeUploader"
        />
        <Renderer
          ref="rendererC"
          antialias
          :orbit-ctrl="{ enableDamping: true }"
          width="800"
          height="600"
        >
          <Camera :position="{ z: 1.5 }" />
          <Scene> </Scene>
        </Renderer>
      </div>
    </div>
    <div
      v-if="selectedMenu == 2"
      class="flex-grow-1 flex align-items-center m-2 justify-content-center"
    >
      <div class="flex flex-column w-full h-full">
        <Toolbar class="mb-2 bg-gray-900">
          <template #start>
            <Button
              @click="startHAL"
              label="Start HAL"
              icon="pi pi-play"
              class="mr-2"
            />
            <Button
              @click="stopHAL"
              label="Stop HAL"
              icon="pi pi-stop"
              severity="success"
            />
          </template>
        </Toolbar>
        <Textarea
          v-model="halStdoutText"
          autoScroll="true"
          rows="30"
          cols="30"
        />
      </div>
    </div>
    <div
      v-if="selectedMenu == 3"
      class="flex-grow-1 flex align-items-center justify-content-center bg-blue-500"
    >
      Settings
    </div>
  </div>
</template>

<style scoped>
.button-mode {
  background: #333;
  color: #ffffff;
}

.button-arrow {
  background: #333;
  color: #ffffff;
  border: none;
  outline: none;
  cursor: none;
  text-decoration: none;
}

.button-direction {
  font-size: 1.5em;
}

.dro-font-mode {
  font-family: "iosevka";
  font-weight: bold;
  font-size: 1.1em;
  text-align: center;
}

.fixed-width-font {
  font-family: "iosevka";
  font-weight: normal;
}

.wrapper,
html,
body {
  width: 100%;
  height: 100%;
  margin: 0;
  color: #ffffff;
  background-color: #222222;
  -webkit-touch-callout: none; /* Safari */
  -webkit-user-select: none; /* Chrome */     
  -moz-user-select: none; /* Firefox */
  -ms-user-select: none; /* Internet Explorer/Edge */
  user-select: none; 
}

.wrapper {
  display: flex;
  flex-direction: column;
}
</style>
