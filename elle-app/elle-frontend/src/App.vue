<script setup lang="ts">
import { ref, onMounted, computed, watch, defineAsyncComponent } from "vue";
import { useDialog } from "primevue/usedialog";
import { Camera, Renderer, RendererPublicInterface, Scene } from "troisjs";

import Numpad from "./components/Numpad.vue";
import DRODisplay from "./components/DRODisplay.vue";
import G76PresetSelector from "./components/G76PresetSelector.vue";
import Backplot from "./Backplot";
import { putHalOut, putGCode, putLinuxCNC, getHalIn, putAbort, putEmergencyStop } from "./HAL"

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
let updateInterval: NodeJS.Timeout;

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

enum CannedCycle {
  none = 0,
  g76 = 1,
  placeholder2 = 2,
  placeholder3 = 3,
  placeholder4 = 4,
}
const selectedCannedCycle = ref(CannedCycle.none);

// G76 Threading Canned Cycle parameters (LinuxCNC spec)
const g76PPitch = ref<number | null>(null);
const g76ZEndPoint = ref<number | null>(null);
const g76IOffset = ref<number | null>(null);
const g76JInitialDepth = ref<number | null>(null);
const g76RDegression = ref<number | null>(null);
const g76KFullDepth = ref<number | null>(null);
const g76QCompoundAngle = ref<number | null>(null);
const g76HSpringPasses = ref<number | null>(null);
const g76ETaperDistance = ref<number | null>(null);
const g76LTaperEnd = ref<number | null>(null);
const g76PresetName = ref<string | null>(null);

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
    case 6: // G76 P
    case 7: // G76 Z
    case 8: // G76 I
    case 9: // G76 J
    case 10: // G76 R
    case 11: // G76 K
    case 14: // G76 E
    case 15: // G76 L
    numberentry.value = numbersPrevious = metric.value ? value : value / 25.4;
    break;
    case 3:
    case 12: // G76 Q
    case 13: // G76 H
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
    case 6: // G76 P
    case 7: // G76 Z
    case 8: // G76 I
    case 9: // G76 J
    case 10: // G76 R
    case 11: // G76 K
    case 14: // G76 E
    case 15: // G76 L
    if (!metric.value) {
      value = value * 25.4;
    }
    break;
    case 3:
    case 12: // G76 Q
    case 13: // G76 H
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
    case 6:
      g76PPitch.value = Math.round(value * 1000000) / 1000000;
      g76PresetName.value = null; // Clear preset name on manual change
      updatePitchFromG76();
      break;
    case 7:
      g76ZEndPoint.value = Math.round(value * 1000000) / 1000000;
      g76PresetName.value = null; // Clear preset name on manual change
      break;
    case 8:
      g76IOffset.value = Math.round(value * 1000000) / 1000000;
      g76PresetName.value = null; // Clear preset name on manual change
      break;
    case 9:
      g76JInitialDepth.value = Math.round(value * 1000000) / 1000000;
      g76PresetName.value = null; // Clear preset name on manual change
      break;
    case 10:
      g76RDegression.value = Math.round(value * 1000000) / 1000000;
      g76PresetName.value = null; // Clear preset name on manual change
      break;
    case 11:
      g76KFullDepth.value = Math.round(value * 1000000) / 1000000;
      g76PresetName.value = null; // Clear preset name on manual change
      break;
    case 12:
      g76QCompoundAngle.value = Math.round(value * 1000000) / 1000000;
      g76PresetName.value = null; // Clear preset name on manual change
      break;
    case 13:
      g76HSpringPasses.value = Math.round(value * 1000000) / 1000000;
      g76PresetName.value = null; // Clear preset name on manual change
      break;
    case 14:
      g76ETaperDistance.value = Math.round(value * 1000000) / 1000000;
      g76PresetName.value = null; // Clear preset name on manual change
      break;
    case 15:
      g76LTaperEnd.value = Math.round(value * 1000000) / 1000000;
      g76PresetName.value = null; // Clear preset name on manual change
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
      if (numbersClicked.length === 0) {
        // Reset G76 fields to null when fully erased
        switch (entryActive.value) {
          case 6:
            g76PPitch.value = null;
            break;
          case 7:
            g76ZEndPoint.value = null;
            break;
          case 8:
            g76IOffset.value = null;
            break;
          case 9:
            g76JInitialDepth.value = null;
            break;
          case 10:
            g76RDegression.value = null;
            break;
          case 11:
            g76KFullDepth.value = null;
            break;
          case 12:
            g76QCompoundAngle.value = null;
            break;
          case 13:
            g76HSpringPasses.value = null;
            break;
          case 14:
            g76ETaperDistance.value = null;
            break;
          case 15:
            g76LTaperEnd.value = null;
            break;
        }
        numpadInputStage = NumpadInputStage.none;
        entryActive.value = 0;
        numberentry.value = 0;
      } else {
        numberentry.value = calcNumber();
      }
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
          xaxisoffset = (halIn as any).position_x - xaxisset;
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
        xpos.value = (halIn as any).position_x - xaxisoffset;
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
        velocity_x_cmd: -velocity,
      };
      putHalOut(halOut);
    }
    if (buttondowntime > 0) {
      halOutScheduled = false;
      let velocity = (Date.now() / 1000 - buttondowntime) * 3;
      velocity = Math.min(velocity, 3.0);
      let halOut = {
        control_x_type: 1,
        velocity_x_cmd: +velocity,
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
      if (selectedMenu.value == 0) {
        let halOut = {
          control_source: false,
          forward_z: zforward ? -zpitch.value : +zpitch.value,
          forward_x: xforward ? +xpitch.value : -xpitch.value,
          enable_z: zpitchactive.value,
          enable_x: xpitchactive.value,
          enable_stepper_z: zstepperactive.value,
          enable_stepper_x: xstepperactive.value,
        };
        putHalOut(halOut);
      } else if (selectedMenu.value == 1) {
        selectedDirectionMode.value = DirectionMode.hold;
        selectedFeedMode.value = FeedMode.backCompound;
        let halOut = {
          control_source: true,
          forward_z: zforward ? -zpitch.value : +zpitch.value,
          forward_x: xforward ? +xpitch.value : -xpitch.value,
          enable_z: true,
          enable_x: true,
          enable_stepper_z: true,
          enable_stepper_x: true,
        };
        putHalOut(halOut);
      }
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

const cannedCycleClicked = (cycle: CannedCycle) => {
  selectedCannedCycle.value = cycle;
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

watch(selectedMenu, () => {
  scheduleHALOut();
});

const PitchSelector = defineAsyncComponent(
  () => import("./components/PitchSelector.vue")
);

// G76PresetSelector is now imported directly above

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

const g76PresetClicked = () => {
  treatOffClickAsEnter();
  entryActive.value = 0;
  const dialogRef = dialog.open(G76PresetSelector, {
    props: {
      header: "Select G76 Thread Preset",
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
    emits: {
      onSelected: (preset: any) => {
        // Preset values are stored in metric units, convert if currently in imperial
        const conversionFactor = metric.value ? 1 : 1/25.4;
        
        g76PPitch.value = Math.round((preset.P * conversionFactor) * 1000000) / 1000000;
        g76ZEndPoint.value = Math.round((preset.Z * conversionFactor) * 1000000) / 1000000;
        g76IOffset.value = Math.round((preset.I * conversionFactor) * 1000000) / 1000000;
        g76JInitialDepth.value = Math.round((preset.J * conversionFactor) * 1000000) / 1000000;
        g76RDegression.value = Math.round(preset.R * 1000000) / 1000000; // Degression factor is dimensionless
        g76KFullDepth.value = Math.round((preset.K * conversionFactor) * 1000000) / 1000000;
        g76QCompoundAngle.value = Math.round(preset.Q * 1000000) / 1000000; // Angle stays the same
        g76HSpringPasses.value = Math.round(preset.H * 1000000) / 1000000; // Number of passes
        g76ETaperDistance.value = Math.round((preset.E * conversionFactor) * 1000000) / 1000000;
        g76LTaperEnd.value = Math.round(preset.L * 1000000) / 1000000; // Taper end specification
        g76PresetName.value = preset.name; // Store the preset name
        updatePitchFromG76();
      },
    },
    templates: {},
    onClose: (options) => {},
  });
};

const g76StartClicked = () => {
  treatOffClickAsEnter();
  entryActive.value = 0;
  
  // Validate required parameters exist (LinuxCNC requires P, Z, K minimum)
  if (g76PPitch.value === null || g76ZEndPoint.value === null || 
      g76KFullDepth.value === null) {
    console.error("G76: Missing required parameters (P, Z, K)");
    alert("Error: Missing required G76 parameters. Please set P (pitch), Z (end point), and K (full depth) values.");
    return;
  }
  
  // Validate parameter ranges and values
  const errors = [];
  
  // P - Thread pitch (should be positive and reasonable)
  if (g76PPitch.value <= 0) {
    errors.push("P (Thread Pitch) must be positive");
  }
  if (g76PPitch.value > 10) {
    errors.push("P (Thread Pitch) seems too large (>10mm), please verify");
  }
  
  // Z - End point (should be negative for typical threading toward chuck)
  if (g76ZEndPoint.value >= 0) {
    errors.push("Z (End Point) should typically be negative (toward chuck)");
  }
  
  // K - Full thread depth (should be positive and reasonable)
  if (g76KFullDepth.value <= 0) {
    errors.push("K (Full Thread Depth) must be positive");
  }
  if (g76KFullDepth.value > 5) {
    errors.push("K (Full Thread Depth) seems too large (>5mm), please verify");
  }
  
  // Validate optional parameters if set
  if (g76IOffset.value !== null && Math.abs(g76IOffset.value) > 10) {
    errors.push("I (Thread Peak Offset) seems too large (>10mm), please verify");
  }
  
  if (g76JInitialDepth.value !== null && (g76JInitialDepth.value <= 0 || g76JInitialDepth.value > g76KFullDepth.value)) {
    errors.push("J (Initial Cut Depth) must be positive and ≤ K (Full Thread Depth)");
  }
  
  if (g76RDegression.value !== null && (g76RDegression.value < 1 || g76RDegression.value > 2)) {
    errors.push("R (Depth Degression) should be between 1.0 and 2.0");
  }
  
  if (g76QCompoundAngle.value !== null && (g76QCompoundAngle.value < 0 || g76QCompoundAngle.value > 90)) {
    errors.push("Q (Compound Slide Angle) should be between 0° and 90°");
  }
  
  if (g76HSpringPasses.value !== null && (g76HSpringPasses.value < 0 || g76HSpringPasses.value > 10)) {
    errors.push("H (Spring Passes) should be between 0 and 10");
  }
  
  // Show errors if any
  if (errors.length > 0) {
    const errorMessage = "G76 Parameter Validation Errors:\n\n" + errors.join("\n");
    console.error("G76 Validation failed:", errors);
    alert(errorMessage);
    return;
  }
  
  // Assemble G76 command with validated parameters (LinuxCNC format)
  let g76Command = `G76 P${g76PPitch.value} Z${g76ZEndPoint.value}`;
  
  // Add optional parameters if they are set and valid
  if (g76IOffset.value !== null && g76IOffset.value !== 0) {
    g76Command += ` I${g76IOffset.value}`;
  }
  
  if (g76JInitialDepth.value !== null && g76JInitialDepth.value !== 0) {
    g76Command += ` J${g76JInitialDepth.value}`;
  }
  
  if (g76RDegression.value !== null && g76RDegression.value !== 0) {
    g76Command += ` R${g76RDegression.value}`;
  }
  
  g76Command += ` K${g76KFullDepth.value}`;
  
  if (g76QCompoundAngle.value !== null && g76QCompoundAngle.value !== 0) {
    g76Command += ` Q${g76QCompoundAngle.value}`;
  }
  
  if (g76HSpringPasses.value !== null && g76HSpringPasses.value !== 0) {
    g76Command += ` H${g76HSpringPasses.value}`;
  }
  
  if (g76ETaperDistance.value !== null && g76ETaperDistance.value !== 0) {
    g76Command += ` E${g76ETaperDistance.value}`;
  }
  
  if (g76LTaperEnd.value !== null && g76LTaperEnd.value !== 0) {
    g76Command += ` L${g76LTaperEnd.value}`;
  }
  
  console.log("G76 Command (LinuxCNC validated):", g76Command);
  
  // Send the validated G76 command to LinuxCNC
  putGCode({ move: "" });
  putGCode({ cycle: g76Command });
};

const g76StopClicked = () => {
  treatOffClickAsEnter();
  entryActive.value = 0;
  
  console.log("G76 Stop clicked");
  
  // Abort current operation (gentler than emergency stop)
  putAbort();
};

const g76ResetClicked = () => {
  treatOffClickAsEnter();
  entryActive.value = 0;
  g76PPitch.value = null;
  g76ZEndPoint.value = null;
  g76IOffset.value = null;
  g76JInitialDepth.value = null;
  g76RDegression.value = null;
  g76KFullDepth.value = null;
  g76QCompoundAngle.value = null;
  g76HSpringPasses.value = null;
  g76ETaperDistance.value = null;
  g76LTaperEnd.value = null;
  g76PresetName.value = null;
  updatePitchFromG76();
};

const updatePitchFromG76 = () => {
  // Update PZ (longitudinal) with thread pitch (P parameter)
  if (g76PPitch.value !== null) {
    zpitch.value = Math.abs(g76PPitch.value);
    if (metric.value) {
      zpitchlabel.value = `${g76PPitch.value}mm`;
    } else {
      // In imperial mode, show TPI (threads per inch)
      const tpi = 1 / g76PPitch.value;
      // Show integer if close to whole number, otherwise 1 decimal place
      const rounded = Math.round(tpi);
      const formatted = Math.abs(tpi - rounded) < 0.1 ? rounded.toString() : tpi.toFixed(1);
      zpitchlabel.value = `${formatted} TPI`;
    }
  } else {
    zpitchlabel.value = "…";
  }
  
  // Update PX (cross) based on compound angle (Q parameter)
  if (g76QCompoundAngle.value !== null && g76QCompoundAngle.value !== 0) {
    // For compound slide angle, calculate cross feed based on angle
    // Q parameter is the compound slide angle in degrees
    const angleRad = g76QCompoundAngle.value * (Math.PI / 180);
    xpitch.value = g76PPitch.value ? Math.abs(g76PPitch.value * Math.tan(angleRad)) : 0.001;
    xpitchlabel.value = `${g76QCompoundAngle.value}°`;
    xpitchangle.value = g76QCompoundAngle.value;
  } else {
    // No compound angle - minimal cross feed for straight threading
    xpitch.value = 0.001; // Very small value for threading
    xpitchlabel.value = "Thread";
    xpitchangle.value = 0;
  }
};

const g76MetricClicked = () => {
  treatOffClickAsEnter();
  metric.value = !metric.value;
  
  // Convert G76 values between metric and imperial
  if (metric.value) {
    // Converting to metric (multiply by 25.4)
    if (g76PPitch.value !== null) g76PPitch.value = Math.round(g76PPitch.value * 25.4 * 1000000) / 1000000;
    if (g76ZEndPoint.value !== null) g76ZEndPoint.value = Math.round(g76ZEndPoint.value * 25.4 * 1000000) / 1000000;
    if (g76QCompoundAngle.value !== null) g76QCompoundAngle.value = Math.round(g76QCompoundAngle.value * 25.4 * 1000000) / 1000000;
    if (g76KFullDepth.value !== null) g76KFullDepth.value = Math.round(g76KFullDepth.value * 25.4 * 1000000) / 1000000;
    if (g76JInitialDepth.value !== null) g76JInitialDepth.value = Math.round(g76JInitialDepth.value * 25.4 * 1000000) / 1000000;
    // A (angle) stays the same
    if (g76PPitch.value !== null) g76PPitch.value = Math.round(g76PPitch.value * 25.4 * 1000000) / 1000000;
  } else {
    // Converting to imperial (divide by 25.4)
    if (g76PPitch.value !== null) g76PPitch.value = Math.round(g76PPitch.value / 25.4 * 1000000) / 1000000;
    if (g76ZEndPoint.value !== null) g76ZEndPoint.value = Math.round(g76ZEndPoint.value / 25.4 * 1000000) / 1000000;
    if (g76QCompoundAngle.value !== null) g76QCompoundAngle.value = Math.round(g76QCompoundAngle.value / 25.4 * 1000000) / 1000000;
    if (g76KFullDepth.value !== null) g76KFullDepth.value = Math.round(g76KFullDepth.value / 25.4 * 1000000) / 1000000;
    if (g76JInitialDepth.value !== null) g76JInitialDepth.value = Math.round(g76JInitialDepth.value / 25.4 * 1000000) / 1000000;
    // A (angle) stays the same
    if (g76PPitch.value !== null) g76PPitch.value = Math.round(g76PPitch.value / 25.4 * 1000000) / 1000000;
  }
  g76PresetName.value = null; // Clear preset name when units change
  updatePitchFromG76();
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
    class="flex flex-row flex-grow-1 absolute top-0 left-0 wrapper"
  >
    <Menu v-model="selectedMenu" :model="menuItems" class="flex-none">
      <template #start>
        <button
          class="w-full p-link flex align-items-center justify-content-start p-2 pl-3 text-color hover:surface-200 border-noround"
        >
          <div class="flex flex-column align">
            <span class="font-bold">Elle</span>
          </div>
        </button>
      </template>
      <template #end>
        <button
          @click="quitApplication"
          class="w-full p-link flex align-items-center justify-content-start p-2 pl-4 text-color hover:surface-200 border-noround"
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
        <div class="divider-vertical"></div>
        <Numpad class="" @numPadClicked="numPadClicked" />
      </div>
      <div class="divider-horizontal"></div>
      <div class="flex flex-row">
        <div
          class="grid dro-font-mode grid-nogutter p-3 pr-4 m-0 mt-2"
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
          class="grid dro-font-mode grid-nogutter p-3 pr-4 m-0 mt-2"
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
          class="grid grid-nogutter mt-2 p-1 dro-font-mode"
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
      class="flex-grow-1 flex flex-column p-2"
    >
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
          @metricClicked="g76MetricClicked"
          @otherClicked="otherClicked"
        />
        <div class="divider-vertical"></div>
        <Numpad class="" @numPadClicked="numPadClicked" />
      </div>
      <div class="divider-horizontal"></div>
      <div class="flex flex-row flex-grow-1">
        <div
          class="grid dro-font-mode grid-nogutter p-3 pr-4 m-0"
          style="width: 18em"
        >
          <div class="col-12 align-content-center">Canned Cycles</div>
          <button
            @click="cannedCycleClicked(CannedCycle.g76)"
            size="large"
            class="col-12 dro-font-mode button-mode p-3 m-1"
          >
            <span class="flex flex-row align-items-center">
              <i
                v-if="selectedCannedCycle == CannedCycle.g76"
                class="pi pi-circle-fill mr-3"
                style="color: #ff0000"
              />
              <i v-else class="pi pi-circle mr-3" />
              G76 Threading Cycle
            </span>
          </button>
          <button
            @click="cannedCycleClicked(CannedCycle.placeholder2)"
            size="large"
            class="col-12 dro-font-mode button-mode p-3 m-1"
          >
            <span class="flex flex-row align-items-center">
              <i
                v-if="selectedCannedCycle == CannedCycle.placeholder2"
                class="pi pi-circle-fill mr-3"
                style="color: #ff0000"
              />
              <i v-else class="pi pi-circle mr-3" />
              Placeholder 2
            </span>
          </button>
          <button
            @click="cannedCycleClicked(CannedCycle.placeholder3)"
            size="large"
            class="col-12 dro-font-mode button-mode p-3 m-1"
          >
            <span class="flex flex-row align-items-center">
              <i
                v-if="selectedCannedCycle == CannedCycle.placeholder3"
                class="pi pi-circle-fill mr-3"
                style="color: #ff0000"
              />
              <i v-else class="pi pi-circle mr-3" />
              Placeholder 3
            </span>
          </button>
          <button
            @click="cannedCycleClicked(CannedCycle.placeholder4)"
            size="large"
            class="col-12 dro-font-mode button-mode p-3 m-1"
          >
            <span class="flex flex-row align-items-center">
              <i
                v-if="selectedCannedCycle == CannedCycle.placeholder4"
                class="pi pi-circle-fill mr-3"
                style="color: #ff0000"
              />
              <i v-else class="pi pi-circle mr-3" />
              Placeholder 4
            </span>
          </button>
        </div>
        <div v-if="selectedCannedCycle == CannedCycle.g76" class="divider-vertical"></div>
        <div v-if="selectedCannedCycle == CannedCycle.g76" class="flex flex-column dro-font-mode p-1" style="width: 43em;">
          <div class="grid grid-nogutter flex-none">
          <div class="col-12 align-content-center mb-2">
            G76 Threading Canned Cycle (LinuxCNC){{ g76PresetName ? ` - ${g76PresetName}` : '' }}
          </div>
          <!-- Left Column -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end">P</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(6, g76PPitch || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-2 truncate', { 'placeholder-text': entryActive != 6 && g76PPitch === null }]"
              :style="{ backgroundColor: entryActive == 6 ? '#666' : '#333' }"
              :title="entryActive == 6 ? String(numberentry) : String(g76PPitch ?? 'Thread pitch')"
            >
              {{ entryActive == 6 ? numberentry : (g76PPitch ?? 'Thread pitch') }}
            </button>
          </div>
          <!-- Right Column -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end">Z</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(7, g76ZEndPoint || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-2 truncate', { 'placeholder-text': entryActive != 7 && g76ZEndPoint === null }]"
              :style="{ backgroundColor: entryActive == 7 ? '#666' : '#333' }"
              :title="entryActive == 7 ? String(numberentry) : String(g76ZEndPoint ?? 'End point of thread')"
            >
              {{ entryActive == 7 ? numberentry : (g76ZEndPoint ?? 'End point of thread') }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          <!-- Left Column -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end">I</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(8, g76IOffset || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-2 truncate', { 'placeholder-text': entryActive != 8 && g76IOffset === null }]"
              :style="{ backgroundColor: entryActive == 8 ? '#666' : '#333' }"
              :title="entryActive == 8 ? String(numberentry) : String(g76IOffset ?? 'Thread peak offset')"
            >
              {{ entryActive == 8 ? numberentry : (g76IOffset ?? 'Thread peak offset') }}
            </button>
          </div>
          <!-- Right Column -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end">J</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(9, g76JInitialDepth || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-2 truncate', { 'placeholder-text': entryActive != 9 && g76JInitialDepth === null }]"
              :style="{ backgroundColor: entryActive == 9 ? '#666' : '#333' }"
              :title="entryActive == 9 ? String(numberentry) : String(g76JInitialDepth ?? 'Initial cut depth')"
            >
              {{ entryActive == 9 ? numberentry : (g76JInitialDepth ?? 'Initial cut depth') }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          <!-- Left Column -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end">R</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(10, g76RDegression || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-2 truncate', { 'placeholder-text': entryActive != 10 && g76RDegression === null }]"
              :style="{ backgroundColor: entryActive == 10 ? '#666' : '#333' }"
              :title="entryActive == 10 ? String(numberentry) : String(g76RDegression ?? 'Depth degression factor')"
            >
              {{ entryActive == 10 ? numberentry : (g76RDegression ?? 'Depth degression factor') }}
            </button>
          </div>
          <!-- Right Column -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end">K</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(11, g76KFullDepth || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-2 truncate', { 'placeholder-text': entryActive != 11 && g76KFullDepth === null }]"
              :style="{ backgroundColor: entryActive == 11 ? '#666' : '#333' }"
              :title="entryActive == 11 ? String(numberentry) : String(g76KFullDepth ?? 'Full thread depth')"
            >
              {{ entryActive == 11 ? numberentry : (g76KFullDepth ?? 'Full thread depth') }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          <!-- Left Column -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end">Q</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(12, g76QCompoundAngle || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-2 truncate', { 'placeholder-text': entryActive != 12 && g76QCompoundAngle === null }]"
              :style="{ backgroundColor: entryActive == 12 ? '#666' : '#333' }"
              :title="entryActive == 12 ? String(numberentry) : String(g76QCompoundAngle ?? 'Compound slide angle')"
            >
              {{ entryActive == 12 ? numberentry : (g76QCompoundAngle ?? 'Compound slide angle') }}
            </button>
          </div>
          <!-- Right Column -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end">H</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(13, g76HSpringPasses || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-2 truncate', { 'placeholder-text': entryActive != 13 && g76HSpringPasses === null }]"
              :style="{ backgroundColor: entryActive == 13 ? '#666' : '#333' }"
              :title="entryActive == 13 ? String(numberentry) : String(g76HSpringPasses ?? 'Spring passes')"
            >
              {{ entryActive == 13 ? numberentry : (g76HSpringPasses ?? 'Spring passes') }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          <!-- Left Column -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end">E</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(14, g76ETaperDistance || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-2 truncate', { 'placeholder-text': entryActive != 14 && g76ETaperDistance === null }]"
              :style="{ backgroundColor: entryActive == 14 ? '#666' : '#333' }"
              :title="entryActive == 14 ? String(numberentry) : String(g76ETaperDistance ?? 'Taper distance')"
            >
              {{ entryActive == 14 ? numberentry : (g76ETaperDistance ?? 'Taper distance') }}
            </button>
          </div>
          <!-- Right Column -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end">L</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(15, g76LTaperEnd || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-2 truncate', { 'placeholder-text': entryActive != 15 && g76LTaperEnd === null }]"
              :style="{ backgroundColor: entryActive == 15 ? '#666' : '#333' }"
              :title="entryActive == 15 ? String(numberentry) : String(g76LTaperEnd ?? 'Taper end spec')"
            >
              {{ entryActive == 15 ? numberentry : (g76LTaperEnd ?? 'Taper end spec') }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          <!-- Preset and Reset buttons -->
          <div class="col-1 p-0"></div>
          <div class="col-2 p-1">
            <button
              @click="g76PresetClicked"
              class="w-full dro-font-mode button-mode p-2"
              style="background: #555; color: #ffffff;"
            >
              ...
            </button>
          </div>
          <div class="col-2 p-1">
            <button
              @click="g76ResetClicked"
              class="w-full dro-font-mode button-mode p-2"
              style="background: #555; color: #ffffff;"
            >
              Reset
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          </div>
          
          <!-- Spacer to push buttons to bottom -->
          <div class="flex-grow-1"></div>
          
          <!-- Start and Stop buttons centered as a group -->
          <div class="flex justify-content-center gap-2 p-1">
            <button
              @click="g76StartClicked"
              class="dro-font-mode button-mode p-2"
              style="background: #22c55e; color: #ffffff; width: 8em;"
            >
              ⏵ Start
            </button>
            <button
              @click="g76StopClicked"
              class="dro-font-mode button-mode p-2"
              style="background: #ef4444; color: #ffffff; width: 8em;"
            >
              ⏹ Stop
            </button>
          </div>
        </div>
      </div>
      <DynamicDialog />
    </div>
    <div
      v-if="selectedMenu == 2"
      class="flex-grow-1 flex flex-column"
    >
      <Toolbar class="flex-none p-1">
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
        class="console-output flex-grow-1"
        spellcheck="false"
        autocomplete="off"
        autocorrect="off"
        autocapitalize="off"
      />
    </div>
    <div
      v-if="selectedMenu == 3"
      class="flex-grow-1 flex align-items-center justify-content-center"
    >
      Settings
    </div>
  </div>
</template>

<style scoped>
.console-output {
  font-family: "iosevka", "Consolas", "Monaco", "Liberation Mono", "Lucida Console", monospace !important;
  font-size: 1em;
  line-height: 1.2;
  height: 100% !important;
  resize: none;
}

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

/* Simple divider lines for visual separation */
.divider-vertical {
  border-left: 1px solid rgba(255, 255, 255, 0.2);
  margin: 0 1rem;
}

.divider-horizontal {
  border-top: 1px solid rgba(255, 255, 255, 0.2);
  margin: 1rem 0;
  width: 100%;
}

.truncate {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.placeholder-text {
  color: #aaaaaa;
}

</style>
 