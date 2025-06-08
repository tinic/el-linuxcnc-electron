<script setup lang="ts">
import { ref, onMounted, computed, watch, defineAsyncComponent } from "vue";
import { useDialog } from "primevue/usedialog";
import Popover from "primevue/popover";
import { Camera, Renderer, RendererPublicInterface, Scene } from "troisjs";

import Numpad from "./components/Numpad.vue";
import DRODisplay from "./components/DRODisplay.vue";
import ThreadPresetSelector from "./components/ThreadPresetSelector.vue";
import Backplot from "./Backplot";
import { putHalOut, putLinuxCNC, getHalIn, putAbort, putEmergencyStop, putThreading } from "./HAL"

const selectedMenu = ref(0);

// Polled over REST
const xpos = ref(0);
const zpos = ref(0);
const apos = ref(0);


// Computed display positions for diameter mode
const displayXPos = computed(() => {
  return diameterMode.value ? xpos.value * 2 : xpos.value;
});

const displayZPos = computed(() => {
  return zpos.value; // Z position is always radius, not affected by diameter mode
});
const rpms = ref(0);
const programRunning = ref(false);
const errorState = ref(false);

// Machine status computed from various states
const machineStatus = computed(() => {
  // Error states take highest priority
  if (errorState.value) {
    return 'error'; // Red - machine error/fault
  }
  
  // Running any cycle takes priority
  if (programRunning.value) {
    return 'running'; // Green - machine running/in cycle
  }
  
  // Mode-based status when not running
  if (selectedMenu.value === 0) {
    return 'manual'; // Blue - manual mode (Home menu)
  } else if (selectedMenu.value === 1) {
    return 'program'; // White - canned cycle mode ready
  }
  
  return 'idle'; // Fallback - should rarely be used
});

const statusDisplay = computed(() => {
  switch (machineStatus.value) {
    case 'error':
      return { text: 'ERROR', class: 'status-error', title: 'Machine: Error/Fault State' };
    case 'running':
      return { text: 'RUNNING', class: 'status-running', title: 'Machine: Running/In Cycle' };
    case 'manual':
      return { text: 'MANUAL', class: 'status-manual', title: 'Machine: Manual Mode' };
    case 'program':
      return { text: 'PROGRAM', class: 'status-program', title: 'Machine: Canned Cycle Mode' };
    case 'idle':
      return { text: 'IDLE', class: 'status-idle', title: 'Machine: Idle' };
    default:
      return { text: 'UNKNOWN', class: 'status-unknown', title: 'Machine: Unknown Status' };
  }
});

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
const diameterMode = ref(false);
const defaultMetricOnStartup = ref(true);
const isQuitting = ref(false);

// Settings functions
const loadSettings = async () => {
  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(" electron/") > -1) {
    try {
      console.log("Attempting to load settings...");
      console.log("window.settings:", window.settings);
      if (window.settings && window.settings.get) {
        const settings = await window.settings.get();
        console.log("Settings loaded successfully:", settings);
        diameterMode.value = settings.diameterMode;
        defaultMetricOnStartup.value = settings.defaultMetricOnStartup;
        metric.value = settings.defaultMetricOnStartup;
      } else {
        console.error("window.settings is not available");
      }
    } catch (error) {
      console.error("Failed to load settings:", error);
    }
  }
};

const saveSettings = async () => {
  if (isQuitting.value) return; // Don't save settings when quitting
  
  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(" electron/") > -1) {
    try {
      await window.settings.save({
        diameterMode: diameterMode.value,
        defaultMetricOnStartup: defaultMetricOnStartup.value,
      });
    } catch (error) {
      console.error("Failed to save settings:", error);
    }
  }
};
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
  threading = 1,
  placeholder2 = 2,
  placeholder3 = 3,
  placeholder4 = 4,
}
const selectedCannedCycle = ref(CannedCycle.none);

// Threading Subroutine parameters (thread-loop.ngc)
// Note: XStart uses current X position, ZStart is always 0
const threadPitch = ref<number | null>(null);
const threadXDepth = ref<number | null>(null);
const threadZDepth = ref<number | null>(null);
const threadAngle = ref<number | null>(null);
const threadZEnd = ref<number | null>(null);
const threadXPullout = ref<number | null>(null);
const threadZPullout = ref<number | null>(null);
const threadFirstCut = ref<number | null>(null);
const threadCutMult = ref<number | null>(null);
const threadMinCut = ref<number | null>(null);
const threadSpringCuts = ref<number | null>(null);
const threadPresetName = ref<string | null>(null);
const threadDiameter = ref<string | null>(null);

// Popover refs and state
const threadingPopovers = ref<Record<string, any>>({});
const currentPopoverLabel = ref("");

// Threading parameter descriptions
const threadingDescriptions: { [key: string]: string } = {
  'P': 'Thread pitch - distance between threads',
  'D': 'Major Diameter / Tap Drill Size\n(major thread diameter and recommended drill size)',
  'XD': 'X Depth - cross-slide cutting depth\n(negative for external, positive for internal)',
  'ZD': 'Z Depth - longitudinal cutting depth\n(zero for straight threads)',
  'A': 'Angle - thread taper angle in degrees\n(zero for straight threads)',
  'ZE': 'Z End - final Z position\n(usually negative for regular right hand threads)',
  'XP': 'X Pullout - cross-slide retract distance\n(positive for external, negative for internal)',
  'ZP': 'Z Pullout - spindle retract distance',
  'FC': 'First Cut - initial cutting depth',
  'CM': 'Cut Multiplier - depth reduction factor\n(0.5-1.0)',
  'MC': 'Min Cut - minimum cutting depth',
  'SC': 'Spring Cuts - number of finishing passes'
};

// Helper function to round threading values with optional unit conversion
const roundThreadValue = (value: number, conversionFactor: number = 1): number => {
  const result = Math.round((value * conversionFactor) * 1000000) / 1000000;
  return parseFloat(result.toFixed(6));
};

// Helper function to format numbers for LinuxCNC (prevents scientific notation)
const formatForLinuxCNC = (value: number): number => {
  return parseFloat(value.toFixed(6));
};

// Show popover with parameter description
const showLabelPopover = (event: Event, labelKey: string) => {
  const popover = threadingPopovers.value[labelKey];
  if (!popover) return;
  
  // If clicking the same label that's currently showing, hide it
  if (currentPopoverLabel.value === labelKey && popover.visible) {
    popover.hide();
    currentPopoverLabel.value = "";
    return;
  }
  
  // Hide any currently visible popover
  if (currentPopoverLabel.value && threadingPopovers.value[currentPopoverLabel.value]) {
    threadingPopovers.value[currentPopoverLabel.value].hide();
  }
  
  // Show the new popover
  currentPopoverLabel.value = labelKey;
  popover.show(event);
};

const menuItems = ref([
  { separator: true },
  {
    label: "Manual",
    icon: "pi pi-fw pi-wrench",
    command: () => {
      selectedMenu.value = 0;
    },
  },
  {
    label: "Program",
    icon: "pi pi-fw pi-cog",
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
    icon: "pi pi-fw pi-sliders-v",
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
    case 1: // X position - handle diameter mode
    numberentry.value = numbersPrevious = metric.value ? (diameterMode.value ? value * 2 : value) : (diameterMode.value ? value * 2 : value) / 25.4;
    break;
    case 2:
    case 4:
    case 5:
    case 6: // Thread Pitch
    case 7: // Thread X Depth
    case 8: // Thread Z Depth
    case 9: // Thread Angle
    case 10: // Thread Z End
    case 11: // Thread X Pullout
    case 12: // Thread Z Pullout
    case 13: // Thread First Cut
    case 15: // Thread Min Cut
    numberentry.value = numbersPrevious = metric.value ? value : value / 25.4;
    break;
    case 3:
    case 14: // Thread Cut Mult (0.8-1.0)
    case 16: // Thread Spring Cuts
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
    case 6: // Thread Pitch
    case 7: // Thread X Depth
    case 8: // Thread Z Depth
    case 9: // Thread Angle
    case 10: // Thread Z End
    case 11: // Thread X Pullout
    case 12: // Thread Z Pullout
    case 13: // Thread First Cut
    case 15: // Thread Min Cut
    if (!metric.value) {
      value = value * 25.4;
    }
    break;
    case 3:
    case 14: // Thread Cut Mult
    case 16: // Thread Spring Cuts
    //nop
    break;
  }
  switch (entryActive.value) {
    case 1:
      // Convert from display value to actual position if in diameter mode
      const actualXValue = diameterMode.value ? value / 2 : value;
      xaxisset = actualXValue;
      xaxissetscheduled = true;
      xpos.value = actualXValue;
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
      threadPitch.value = roundThreadValue(value);
      threadPresetName.value = null; // Clear preset name on manual change
      threadDiameter.value = null;
      updatePitchFromThread();
      break;
    case 7:
      threadXDepth.value = roundThreadValue(value);
      threadPresetName.value = null; // Clear preset name on manual change
      threadDiameter.value = null;
      break;
    case 8:
      threadZDepth.value = roundThreadValue(value);
      threadPresetName.value = null; // Clear preset name on manual change
      threadDiameter.value = null;
      break;
    case 9:
      threadAngle.value = roundThreadValue(value);
      threadPresetName.value = null; // Clear preset name on manual change
      threadDiameter.value = null;
      break;
    case 10:
      threadZEnd.value = roundThreadValue(value);
      threadPresetName.value = null; // Clear preset name on manual change
      threadDiameter.value = null;
      break;
    case 11:
      threadXPullout.value = roundThreadValue(value);
      threadPresetName.value = null; // Clear preset name on manual change
      threadDiameter.value = null;
      break;
    case 12:
      threadZPullout.value = roundThreadValue(value);
      threadPresetName.value = null; // Clear preset name on manual change
      threadDiameter.value = null;
      break;
    case 13:
      threadFirstCut.value = roundThreadValue(value);
      threadPresetName.value = null; // Clear preset name on manual change
      threadDiameter.value = null;
      break;
    case 14:
      threadCutMult.value = roundThreadValue(value);
      threadPresetName.value = null; // Clear preset name on manual change
      threadDiameter.value = null;
      break;
    case 15:
      threadMinCut.value = roundThreadValue(value);
      threadPresetName.value = null; // Clear preset name on manual change
      threadDiameter.value = null;
      break;
    case 16:
      threadSpringCuts.value = roundThreadValue(value);
      threadPresetName.value = null; // Clear preset name on manual change
      threadDiameter.value = null;
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
        // Reset threading fields to null when fully erased
        switch (entryActive.value) {
          case 6:
            threadPitch.value = null;
            break;
          case 7:
            threadXDepth.value = null;
            break;
          case 8:
            threadZDepth.value = null;
            break;
          case 9:
            threadAngle.value = null;
            break;
          case 10:
            threadZEnd.value = null;
            break;
          case 11:
            threadXPullout.value = null;
            break;
          case 12:
            threadZPullout.value = null;
            break;
          case 13:
            threadFirstCut.value = null;
            break;
          case 14:
            threadCutMult.value = null;
            break;
          case 15:
            threadMinCut.value = null;
            break;
          case 16:
            threadSpringCuts.value = null;
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
    xpos.value = 0;
    zpos.value = 0;
    xaxisoffset = 0;
    zaxisoffset = 0;
  }
};

const stopHAL = () => {
  halStdoutText.value = "";
  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(" electron/") > -1) {
    window.api.send("stopHAL");
    xpos.value = 0;
    zpos.value = 0;
    xaxisoffset = 0;
    zaxisoffset = 0;
  }
};

const quitApplication = async () => {
  var userAgent = navigator.userAgent.toLowerCase();
  
  if (userAgent.indexOf(" electron/") > -1) {
    isQuitting.value = true;
    
    // Save settings one final time before quitting
    try {
      await window.settings.save({
        diameterMode: diameterMode.value,
        defaultMetricOnStartup: defaultMetricOnStartup.value,
      });
    } catch (error) {
      console.error("Failed to save final settings:", error);
    }
    
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
        programRunning.value = (halIn as any).program_running || false;
        errorState.value = (halIn as any).error_state || false;
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

// Watch settings and save them when they change
watch([diameterMode, defaultMetricOnStartup], () => {
  saveSettings();
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

const threadPresetClicked = () => {
  treatOffClickAsEnter();
  entryActive.value = 0;
  const dialogRef = dialog.open(ThreadPresetSelector, {
    props: {
      header: "Select Threading Preset",
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
        
        // Note: XStart uses current position, ZStart is always 0, XEndOffset is 0 for straight threads
        threadPitch.value = roundThreadValue(preset.Pitch, conversionFactor);
        threadXDepth.value = roundThreadValue(preset.XDepth, conversionFactor);
        threadZDepth.value = roundThreadValue(preset.ZDepth, conversionFactor);
        threadAngle.value = roundThreadValue(preset.Angle || 0); // Angle is dimensionless
        threadZEnd.value = roundThreadValue(preset.ZEnd, conversionFactor);
        threadXPullout.value = roundThreadValue(preset.XPullout, conversionFactor);
        threadZPullout.value = roundThreadValue(preset.ZPullout, conversionFactor);
        threadFirstCut.value = roundThreadValue(preset.FirstCut, conversionFactor);
        threadCutMult.value = roundThreadValue(preset.CutMult); // Dimensionless
        threadMinCut.value = roundThreadValue(preset.MinCut, conversionFactor);
        threadSpringCuts.value = roundThreadValue(preset.SpringCuts); // Number of passes
        threadPresetName.value = preset.name; // Store the preset name
        threadDiameter.value = preset.Diameter || null; // Store diameter info (informational only)
        updatePitchFromThread();
      },
    },
    templates: {},
    onClose: (options) => {},
  });
};

const threadStartClicked = () => {
  treatOffClickAsEnter();
  entryActive.value = 0;
  
  // Validate required parameters exist for thread-loop.ngc
  if (threadPitch.value === null || threadZEnd.value === null) {
    console.error("Threading: Missing required parameters");
    alert("Error: Missing required threading parameters. Please set Pitch and Z End values.");
    return;
  }
  
  // Validate parameter ranges and values
  const errors = [];
  
  // Pitch validation
  if (threadPitch.value <= 0) {
    errors.push("Pitch must be positive");
  }
  if (threadPitch.value > 10) {
    errors.push("Pitch seems too large (>10mm), please verify");
  }
  
  // Optional parameter validation
  if (threadXDepth.value !== null && Math.abs(threadXDepth.value) > 10) {
    errors.push("X Depth seems too large (>10mm), please verify");
  }
  
  if (threadFirstCut.value !== null && threadFirstCut.value <= 0) {
    errors.push("First Cut must be positive");
  }
  
  if (threadCutMult.value !== null && (threadCutMult.value < 0.5 || threadCutMult.value > 1.0)) {
    errors.push("Cut Multiplier should be between 0.5 and 1.0");
  }
  
  if (threadMinCut.value !== null && threadMinCut.value <= 0) {
    errors.push("Min Cut must be positive");
  }
  
  if (threadSpringCuts.value !== null && (threadSpringCuts.value < 0 || threadSpringCuts.value > 10)) {
    errors.push("Spring Cuts should be between 0 and 10");
  }
  
  // Show errors if any
  if (errors.length > 0) {
    const errorMessage = "Threading Parameter Validation Errors:\n\n" + errors.join("\n");
    console.error("Threading Validation failed:", errors);
    alert(errorMessage);
    return;
  }
   
  getHalIn().then((halIn) => {
    // Extract current position and threading values into local variables
    // Use HAL positions as source of truth for machine position
    const currentXPos = (halIn as any).position_x - xaxisoffset;
    const currentZPos = (halIn as any).position_z - zaxisoffset;
    const currentAPos = (halIn as any).position_a - aaxisoffset;
    
    const pitch = threadPitch.value || 0;
    const xDepth = threadXDepth.value || 0;
    const zDepth = threadZDepth.value || 0;
    const angle = threadAngle.value || 0;
    const zEnd = threadZEnd.value || 0;
    const xPullout = threadXPullout.value || 0.1;
    const zPullout = threadZPullout.value || 0.1;
    const firstCut = threadFirstCut.value || 0.1;
    const cutMult = threadCutMult.value || 0.8;
    const minCut = threadMinCut.value || 0.05;
    const springCuts = threadSpringCuts.value || 1;
    
    // Calculate lead-in distance (was previously done in thread_loop.ngc)
    const leadIn = pitch * 4;
    
    // For miter threads, calculate X positions at actual start and end positions
    // The user-specified positions are where cutting should occur
    const userZStart = 0; // Current Z position
    const userZEnd = zEnd; // User-specified end position
    const actualThreadLength = Math.abs(userZEnd - userZStart + zDepth);
    
    // Calculate X offset for the actual thread cutting distance (without lead-in)
    const xEndOffset = angle !== 0 ? actualThreadLength * Math.tan(angle * Math.PI / 180) : 0;
    
    // Calculate start position accounting for lead-in
    const actualZStart = userZStart + leadIn; // Move back by lead-in
    const actualXStart = angle !== 0 ? leadIn * Math.tan(angle * Math.PI / 180) : 0;
    
    // Calculate end position for tool movement (where tool actually moves to)
    const toolZEnd = userZEnd - leadIn; // Tool end includes lead-out
    const toolXEnd = xEndOffset; // X end offset for the cutting portion
    
    // Send threading parameters to LinuxCNC HAL component for subroutine call
    const threadingParams = {
      XPos: formatForLinuxCNC(currentXPos),
      ZPos: formatForLinuxCNC(currentZPos),
      APos: formatForLinuxCNC(currentAPos),
      XStart: formatForLinuxCNC(currentXPos - actualXStart), // Adjusted start position for miter
      ZStart: formatForLinuxCNC(currentZPos + actualZStart), // Start position with lead-in
      Pitch: formatForLinuxCNC(pitch),
      XDepth: formatForLinuxCNC(xDepth),
      ZDepth: formatForLinuxCNC(zDepth),
      XEnd: formatForLinuxCNC(currentXPos + xEndOffset), // Where cutting ends (user-specified end)
      ZEnd: formatForLinuxCNC(currentZPos + userZEnd), // Where cutting ends (user-specified end)
      XReturn: formatForLinuxCNC(currentXPos), // Return to original X position
      ZReturn: formatForLinuxCNC(currentZPos), // Return to original Z position
      XPullout: formatForLinuxCNC(xPullout),
      ZPullout: formatForLinuxCNC(zPullout),
      FirstCut: formatForLinuxCNC(firstCut),
      CutMult: formatForLinuxCNC(cutMult),
      MinCut: formatForLinuxCNC(minCut),
      SpringCuts: Math.round(springCuts)
    };
    
    console.log("Threading Parameters:", threadingParams);
    
    // Send to HAL component to set parameters and call subroutine
    putThreading(threadingParams);
  });
};

const threadStopClicked = () => {
  treatOffClickAsEnter();
  entryActive.value = 0;
  
  console.log("Threading Stop clicked");
  
  // Abort current operation (gentler than emergency stop)
  putAbort();
};

const threadResetClicked = () => {
  treatOffClickAsEnter();
  entryActive.value = 0;
  threadPitch.value = null;
  threadXDepth.value = null;
  threadZDepth.value = null;
  threadAngle.value = null;
  threadZEnd.value = null;
  threadXPullout.value = null;
  threadZPullout.value = null;
  threadFirstCut.value = null;
  threadCutMult.value = null;
  threadMinCut.value = null;
  threadSpringCuts.value = null;
  threadPresetName.value = null;
  threadDiameter.value = null;
  updatePitchFromThread();
};

const updatePitchFromThread = () => {
  // Update PZ (longitudinal) with thread pitch parameter
  if (threadPitch.value !== null) {
    zpitch.value = Math.abs(threadPitch.value);
    if (metric.value) {
      zpitchlabel.value = `${threadPitch.value}mm`;
    } else {
      // In imperial mode, show TPI (threads per inch)
      const tpi = 1 / threadPitch.value;
      // Show integer if close to whole number, otherwise 1 decimal place
      const rounded = Math.round(tpi);
      const formatted = Math.abs(tpi - rounded) < 0.1 ? rounded.toString() : tpi.toFixed(1);
      zpitchlabel.value = `${formatted} TPI`;
    }
  } else {
    zpitchlabel.value = "…";
  }
  
  // For thread-loop, if we have Z depth (compound threading), calculate cross feed
  if (threadZDepth.value !== null && threadZDepth.value !== 0 && threadXDepth.value !== null) {
    // Calculate compound angle from X and Z depths
    const angle = Math.atan2(Math.abs(threadZDepth.value), Math.abs(threadXDepth.value)) * (180 / Math.PI);
    const angleRad = angle * (Math.PI / 180);
    xpitch.value = threadPitch.value ? Math.abs(threadPitch.value * Math.tan(angleRad)) : 0.001;
    xpitchlabel.value = `${angle.toFixed(1)}°`;
    xpitchangle.value = angle;
  } else {
    // Straight threading - minimal cross feed
    xpitch.value = 0.001; // Very small value for threading
    xpitchlabel.value = "Thread";
    xpitchangle.value = 0;
  }
};

const threadMetricClicked = () => {
  treatOffClickAsEnter();
  metric.value = !metric.value;
  
  // Convert thread-loop values between metric and imperial
  const conversionFactor = metric.value ? 25.4 : 1/25.4;
  
  // Convert dimensional parameters (those affected by unit conversion)
  if (threadPitch.value !== null) threadPitch.value = roundThreadValue(threadPitch.value, conversionFactor);
  if (threadXDepth.value !== null) threadXDepth.value = roundThreadValue(threadXDepth.value, conversionFactor);
  if (threadZDepth.value !== null) threadZDepth.value = roundThreadValue(threadZDepth.value, conversionFactor);
  // Angle is dimensionless - no conversion needed
  if (threadZEnd.value !== null) threadZEnd.value = roundThreadValue(threadZEnd.value, conversionFactor);
  if (threadXPullout.value !== null) threadXPullout.value = roundThreadValue(threadXPullout.value, conversionFactor);
  if (threadZPullout.value !== null) threadZPullout.value = roundThreadValue(threadZPullout.value, conversionFactor);
  if (threadFirstCut.value !== null) threadFirstCut.value = roundThreadValue(threadFirstCut.value, conversionFactor);
  if (threadMinCut.value !== null) threadMinCut.value = roundThreadValue(threadMinCut.value, conversionFactor);
  // CutMult and SpringCuts are dimensionless - no conversion needed
  threadPresetName.value = null; // Clear preset name when units change
  threadDiameter.value = null; // Clear diameter info when units change
  updatePitchFromThread();
};

onMounted(async () => {
  // Load settings first
  await loadSettings();
  
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
        <div class="flex flex-column">
          <!-- Industrial Stack Light -->
          <div class="flex align-items-center justify-content-start p-2 pl-4 border-noround">
            <div class="stack-light" :title="statusDisplay.title">
              <div class="stack-light-base"></div>
              <div :class="['stack-segment', 'segment-red', { 'active': machineStatus === 'error' }]"></div>
              <div :class="['stack-segment', 'segment-amber', { 'active': false }]"></div>
              <div :class="['stack-segment', 'segment-green', { 'active': machineStatus === 'running' }]"></div>
              <div :class="['stack-segment', 'segment-white', { 'active': machineStatus === 'program' }]"></div>
              <div :class="['stack-segment', 'segment-blue', { 'active': machineStatus === 'manual' }]"></div>
            </div>
            <span class="ml-3 text-sm font-semibold">{{ statusDisplay.text }}</span>
          </div>
          <div class="menu-separator"></div>
          <button
            @click="quitApplication"
            class="w-full p-link flex align-items-center justify-content-start p-2 pl-4 text-color hover:surface-200 border-noround"
          >
            <i class="pi pi-sign-out" />
            <span class="ml-2">Exit</span>
          </button>
        </div>
      </template>
    </Menu>
    <div v-if="selectedMenu == 0" class="m-2">
      <div class="flex flex-row">
        <DRODisplay
          class="mr-2 h-min"
          :entryActive="entryActive"
          :xpos="displayXPos"
          :zpos="displayZPos"
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
          :diameterMode="diameterMode"
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
          :xpos="displayXPos"
          :zpos="displayZPos"
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
          :diameterMode="diameterMode"
          @numberClicked="numberClicked"
          @zeroClicked="zeroClicked"
          @pitchClicked="pitchClicked"
          @metricClicked="threadMetricClicked"
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
          <div class="col-12 align-content-center">Program</div>
          <button
            @click="cannedCycleClicked(CannedCycle.threading)"
            size="large"
            class="col-12 dro-font-mode button-mode p-3 m-1"
          >
            <span class="flex flex-row align-items-center">
              <i
                v-if="selectedCannedCycle == CannedCycle.threading"
                class="pi pi-circle-fill mr-3"
                style="color: #ff0000"
              />
              <i v-else class="pi pi-circle mr-3" />
              Threading
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
        <div v-if="selectedCannedCycle == CannedCycle.threading" class="divider-vertical"></div>
        <div v-if="selectedCannedCycle == CannedCycle.threading" class="flex flex-column dro-font-mode p-1" style="width: 43em;">
          <div class="grid grid-nogutter flex-none">
          <div class="col-12 align-content-center mb-2">
            Threading (thread-loop.ngc){{ threadPresetName ? ` - ${threadPresetName}` : '' }}
          </div>
          <!-- Note: X Start uses current position, Z Start is always 0 -->
          
          <!-- Row 1 -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'P')">P</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(6, threadPitch || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != 6 && threadPitch === null }]"
              :style="{ backgroundColor: entryActive == 6 ? '#666' : '#333' }"
              :title="entryActive == 6 ? String(numberentry) : String(threadPitch ?? 'Pitch')"
            >
              {{ entryActive == 6 ? numberentry : (threadPitch ?? 'Pitch') }}
            </button>
          </div>
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'D')">D</div>
          <div class="col-4 p-1">
            <div
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': threadDiameter === null }]"
              :style="{ backgroundColor: '#333', color: '#999', cursor: 'default' }"
              :title="threadDiameter ?? 'Major Ø / Drill Size'"
            >
              {{ threadDiameter ?? 'Major Ø / Drill' }}
            </div>
          </div>
          <div class="col-2 p-0"></div>
          
          <!-- Row 2 -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'ZD')">ZD</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(8, threadZDepth || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != 8 && threadZDepth === null }]"
              :style="{ backgroundColor: entryActive == 8 ? '#666' : '#333' }"
              :title="entryActive == 8 ? String(numberentry) : String(threadZDepth ?? 'Z Depth')"
            >
              {{ entryActive == 8 ? numberentry : (threadZDepth ?? 'Z Depth') }}
            </button>
          </div>
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'XD')">XD</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(7, threadXDepth || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != 7 && threadXDepth === null }]"
              :style="{ backgroundColor: entryActive == 7 ? '#666' : '#333' }"
              :title="entryActive == 7 ? String(numberentry) : String(threadXDepth ?? 'X Depth')"
            >
              {{ entryActive == 7 ? numberentry : (threadXDepth ?? 'X Depth') }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          <!-- Row 3 -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'ZE')">ZE</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(10, threadZEnd || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != 10 && threadZEnd === null }]"
              :style="{ backgroundColor: entryActive == 10 ? '#666' : '#333' }"
              :title="entryActive == 10 ? String(numberentry) : String(threadZEnd ?? 'Z End')"
            >
              {{ entryActive == 10 ? numberentry : (threadZEnd ?? 'Z End') }}
            </button>
          </div>
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'A')">A</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(9, threadAngle || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != 9 && threadAngle === null }]"
              :style="{ backgroundColor: entryActive == 9 ? '#666' : '#333' }"
              :title="entryActive == 9 ? String(numberentry) : String(threadAngle ?? 'Angle')"
            >
              {{ entryActive == 9 ? numberentry : (threadAngle ?? 'Angle') }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          <!-- Row 4 -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'ZP')">ZP</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(12, threadZPullout || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != 12 && threadZPullout === null }]"
              :style="{ backgroundColor: entryActive == 12 ? '#666' : '#333' }"
              :title="entryActive == 12 ? String(numberentry) : String(threadZPullout ?? 'Z Pullout')"
            >
              {{ entryActive == 12 ? numberentry : (threadZPullout ?? 'Z Pullout') }}
            </button>
          </div>
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'XP')">XP</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(11, threadXPullout || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != 11 && threadXPullout === null }]"
              :style="{ backgroundColor: entryActive == 11 ? '#666' : '#333' }"
              :title="entryActive == 11 ? String(numberentry) : String(threadXPullout ?? 'X Pullout')"
            >
              {{ entryActive == 11 ? numberentry : (threadXPullout ?? 'X Pullout') }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          <!-- Row 5 -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'CM')">CM</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(14, threadCutMult || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != 14 && threadCutMult === null }]"
              :style="{ backgroundColor: entryActive == 14 ? '#666' : '#333' }"
              :title="entryActive == 14 ? String(numberentry) : String(threadCutMult ?? 'Cut Multiplier')"
            >
              {{ entryActive == 14 ? numberentry : (threadCutMult ?? 'Cut Multiplier') }}
            </button>
          </div>
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'FC')">FC</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(13, threadFirstCut || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != 13 && threadFirstCut === null }]"
              :style="{ backgroundColor: entryActive == 13 ? '#666' : '#333' }"
              :title="entryActive == 13 ? String(numberentry) : String(threadFirstCut ?? 'First Cut')"
            >
              {{ entryActive == 13 ? numberentry : (threadFirstCut ?? 'First Cut') }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          <!-- Row 6 -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'SC')">SC</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(16, threadSpringCuts || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != 16 && threadSpringCuts === null }]"
              :style="{ backgroundColor: entryActive == 16 ? '#666' : '#333' }"
              :title="entryActive == 16 ? String(numberentry) : String(threadSpringCuts ?? 'Spring Cuts')"
            >
              {{ entryActive == 16 ? numberentry : (threadSpringCuts ?? 'Spring Cuts') }}
            </button>
          </div>
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'MC')">MC</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(15, threadMinCut || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != 15 && threadMinCut === null }]"
              :style="{ backgroundColor: entryActive == 15 ? '#666' : '#333' }"
              :title="entryActive == 15 ? String(numberentry) : String(threadMinCut ?? 'Min Cut')"
            >
              {{ entryActive == 15 ? numberentry : (threadMinCut ?? 'Min Cut') }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          
          
          </div>
          
          <!-- Spacer to push buttons to bottom -->
          <div class="flex-grow-1"></div>
          
          <!-- Start, Stop, Preset and Reset buttons centered as a group -->
          <div class="flex justify-content-center gap-2 p-1">
            <button
              @click="threadPresetClicked"
              class="dro-font-mode button-mode p-2"
              style="background: #555; color: #ffffff; width: 6em;"
            >
              ...
            </button>
            <button
              @click="threadResetClicked"
              class="dro-font-mode button-mode p-2"
              style="background: #555; color: #ffffff; width: 6em;"
            >
              Reset
            </button>
            <button
              @click="threadStartClicked"
              class="dro-font-mode button-mode p-2"
              style="background: #22c55e; color: #ffffff; width: 6em;"
            >
              ⏵ Start
            </button>
            <button
              @click="threadStopClicked"
              class="dro-font-mode button-mode p-2"
              style="background: #ef4444; color: #ffffff; width: 6em;"
            >
              ⏹ Stop
            </button>
          </div>
          
          <!-- Threading Label Popovers -->
          <Popover :ref="(el) => threadingPopovers['P'] = el" class="threading-popover">
            <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line;">{{ threadingDescriptions['P'] }}</div>
          </Popover>
          <Popover :ref="(el) => threadingPopovers['D'] = el" class="threading-popover">
            <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line;">{{ threadingDescriptions['D'] }}</div>
          </Popover>
          <Popover :ref="(el) => threadingPopovers['XD'] = el" class="threading-popover">
            <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line;">{{ threadingDescriptions['XD'] }}</div>
          </Popover>
          <Popover :ref="(el) => threadingPopovers['ZD'] = el" class="threading-popover">
            <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line;">{{ threadingDescriptions['ZD'] }}</div>
          </Popover>
          <Popover :ref="(el) => threadingPopovers['A'] = el" class="threading-popover">
            <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line;">{{ threadingDescriptions['A'] }}</div>
          </Popover>
          <Popover :ref="(el) => threadingPopovers['ZE'] = el" class="threading-popover">
            <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line;">{{ threadingDescriptions['ZE'] }}</div>
          </Popover>
          <Popover :ref="(el) => threadingPopovers['XP'] = el" class="threading-popover">
            <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line;">{{ threadingDescriptions['XP'] }}</div>
          </Popover>
          <Popover :ref="(el) => threadingPopovers['ZP'] = el" class="threading-popover">
            <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line;">{{ threadingDescriptions['ZP'] }}</div>
          </Popover>
          <Popover :ref="(el) => threadingPopovers['FC'] = el" class="threading-popover">
            <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line;">{{ threadingDescriptions['FC'] }}</div>
          </Popover>
          <Popover :ref="(el) => threadingPopovers['CM'] = el" class="threading-popover">
            <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line;">{{ threadingDescriptions['CM'] }}</div>
          </Popover>
          <Popover :ref="(el) => threadingPopovers['MC'] = el" class="threading-popover">
            <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line;">{{ threadingDescriptions['MC'] }}</div>
          </Popover>
          <Popover :ref="(el) => threadingPopovers['SC'] = el" class="threading-popover">
            <div class="p-2 dro-font-mode text-sm" style="white-space: pre-line;">{{ threadingDescriptions['SC'] }}</div>
          </Popover>
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
      class="flex-grow-1 p-4"
    >
      <div class="dro-font-mode">
        <h2 class="mb-4">Settings</h2>
        
        <div class="grid grid-nogutter">
          <!-- Diameter Mode Setting -->
          <div class="col-12 mb-4">
            <div class="grid grid-nogutter align-items-center">
              <div class="col-6 text-right pr-4">
                <label class="text-lg font-semibold">Display Mode:</label>
              </div>
              <div class="col-6">
                <div class="flex gap-3">
                  <button
                    @click="diameterMode = false"
                    :class="['button-mode p-2 px-4', { 'bg-primary': !diameterMode }]"
                  >
                    Radius
                  </button>
                  <button
                    @click="diameterMode = true"
                    :class="['button-mode p-2 px-4', { 'bg-primary': diameterMode }]"
                  >
                    Diameter
                  </button>
                </div>
              </div>
            </div>
          </div>
          
          <!-- Default Units Setting -->
          <div class="col-12 mb-4">
            <div class="grid grid-nogutter align-items-center">
              <div class="col-6 text-right pr-4">
                <label class="text-lg font-semibold">Default Units:</label>
              </div>
              <div class="col-6">
                <div class="flex gap-3">
                  <button
                    @click="defaultMetricOnStartup = true"
                    :class="['button-mode p-2 px-4', { 'bg-primary': defaultMetricOnStartup }]"
                  >
                    Metric (mm)
                  </button>
                  <button
                    @click="defaultMetricOnStartup = false"
                    :class="['button-mode p-2 px-4', { 'bg-primary': !defaultMetricOnStartup }]"
                  >
                    Imperial (inch)
                  </button>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.console-output {
  font-family: "iosevka" !important;
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

/* Industrial Stack Light Design */
.stack-light {
  position: relative;
  display: flex;
  flex-direction: column;
  align-items: center;
}

.stack-light-base {
  width: 36px;
  height: 12px;
  background: #444;
  border-radius: 0 0 6px 6px;
  margin-top: 3px;
}

.stack-segment {
  width: 30px;
  height: 18px;
  margin: 3px 0;
  border-radius: 50%;
  border: 3px solid #333;
  transition: all 0.3s ease;
  position: relative;
}

/* Inactive (darkened) states */
.segment-red {
  background: #441a1a;
  border-color: #662222;
}

.segment-amber {
  background: #442a0a;
  border-color: #664411;
}

.segment-green {
  background: #1a441a;
  border-color: #226622;
}

.segment-blue {
  background: #1a2244;
  border-color: #223366;
}

.segment-white {
  background: #444444;
  border-color: #666666;
}

/* Active (illuminated) states with glow effect */
.segment-red.active {
  background: #ef4444;
  border-color: #dc2626;
  box-shadow: 0 0 24px #ef4444, inset 0 3px 6px rgba(255,255,255,0.3);
}

.segment-amber.active {
  background: #f59e0b;
  border-color: #d97706;
  box-shadow: 0 0 24px #f59e0b, inset 0 3px 6px rgba(255,255,255,0.3);
}

.segment-green.active {
  background: #22c55e;
  border-color: #16a34a;
  box-shadow: 0 0 24px #22c55e, inset 0 3px 6px rgba(255,255,255,0.3);
}

.segment-blue.active {
  background: #3b82f6;
  border-color: #2563eb;
  box-shadow: 0 0 24px #3b82f6, inset 0 3px 6px rgba(255,255,255,0.3);
}

.segment-white.active {
  background: #ffffff;
  border-color: #e5e7eb;
  box-shadow: 0 0 24px #ffffff, inset 0 3px 6px rgba(255,255,255,0.4);
}

/* Threading popover styling */
:deep(.threading-popover .p-popover) {
  background-color: #333 !important;
  border: 1px solid #555 !important;
  color: #ffffff !important;
  font-size: 0.9em;
  max-width: 250px;
}

:deep(.threading-popover .p-popover-arrow) {
  border-bottom-color: #333 !important;
  border-top-color: #333 !important;
  border-left-color: #333 !important;
  border-right-color: #333 !important;
}

.cursor-pointer {
  cursor: pointer;
}

.cursor-pointer:hover {
  color: #aaaaaa;
}

.menu-separator {
  height: 1px;
  background-color: rgba(255, 255, 255, 0.12);
  margin: 0.5rem 0;
}

.bg-primary {
  background-color: #3b82f6 !important;
  color: white !important;
}

/* Debug coordinates styling */
.debug-coordinates {
  font-family: "iosevka", monospace;
  font-size: 0.75rem;
  line-height: 1.2;
  border-left: 2px solid #555;
  margin-left: 1rem;
}

.coordinate-line {
  margin-bottom: 0.25rem;
  color: #cccccc;
}

.text-color-secondary {
  color: #888888;
  font-weight: bold;
}

</style>
 