<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed, watch, defineAsyncComponent } from "vue";
import { useDialog } from "primevue/usedialog";
import Popover from "primevue/popover";
import Dialog from "primevue/dialog";
import { Camera, Renderer, RendererPublicInterface, Scene } from "troisjs";
import * as THREE from "three";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls";

import Numpad from "./components/Numpad.vue";
import DRODisplay from "./components/DRODisplay.vue";
import ThreadPresetSelector from "./components/ThreadPresetSelector.vue";
import OperationPreview from "./components/OperationPreview.vue";
import Backplot from "./Backplot";
import { useHAL } from "./composables/useHAL";
import { useCannedCycles, ThreadingEntryType } from "./composables/useCannedCycles";

enum EntryType {
  xPosition = 1,
  zPosition = 2,
  aPosition = 3,
  xPitch = 4,
  zPitch = 5
}

enum MenuType {
  manual = 0,
  cannedCycles = 1,
  halStatus = 2,
  settings = 3
}

const selectedMenu = ref(MenuType.manual);

const {
  xpos,
  zpos,
  apos,
  rpms,
  cannedCycleRunning,
  errorState,
  putHalOut,
  putLinuxCNC,
  getHalIn,
  putAbort,
  putEmergencyStop,
  putThreading,
  generateThreadingGcode,
  cleanupCannedCycles,
  startPoll,
  endPoll,
  stopJogNow,
  setAxisOffset,
  scheduleHALOut,
  setButtonTime,
  scheduleButtonUp,
  getAxisOffset,
  setAxisValue,
  xpitch,
  zpitch,
  xpitchactive,
  zpitchactive,
  xstepperactive,
  zstepperactive,
  updateHALOut
} = useHAL();

const {
  threadPitch,
  threadXDepth,
  threadZDepth,
  threadAngle,
  threadZEnd,
  threadXPullout,
  threadZPullout,
  threadFirstCut,
  threadCutMult,
  threadMinCut,
  threadSpringCuts,
  threadPresetName,
  threadDiameter,
  threadingPopovers,
  currentPopoverLabel,
  threadingDescriptions,
  roundThreadValue,
  formatForLinuxCNC,
  showLabelPopover,
  validateThreadingParameters,
  generateThreadingParams,
  openThreadPresetDialog,
  resetThreadingParameters,
  convertThreadingParameters,
  setThreadingParameter,
  clearThreadingParameter
} = useCannedCycles();


// Computed display positions for diameter mode
const displayXPos = computed(() => {
  return diameterMode.value ? xpos.value * 2 : xpos.value;
});

const displayZPos = computed(() => {
  return zpos.value; // Z position is always radius, not affected by diameter mode
});

// Machine status computed from various states
const machineStatus = computed(() => {
  // Error states take highest priority
  if (errorState.value) {
    return 'error'; // Red - machine error/fault
  }
  
  // Running any cycle takes priority
  if (cannedCycleRunning.value) {
    return 'running'; // Green - machine running/in cycle
  }
  
  // Mode-based status when not running
  if (selectedMenu.value === MenuType.manual) {
    return 'manual'; // Blue - manual mode (Home menu)
  } else if (selectedMenu.value === MenuType.cannedCycles) {
    return 'cannedCycle'; // White - canned cycle mode ready
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
    case 'cannedCycle':
      return { text: 'CANNED CYCLE', class: 'status-cannedCycle', title: 'Machine: Canned Cycle Mode' };
    case 'idle':
      return { text: 'IDLE', class: 'status-idle', title: 'Machine: Idle' };
    default:
      return { text: 'UNKNOWN', class: 'status-unknown', title: 'Machine: Unknown Status' };
  }
});

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
      if (window.settings && window.settings.get) {
        const settings = await window.settings.get();
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



// Operation Preview state
const showOperationPreview = ref(false);
const showBackplot = ref(false);
const currentOperation = ref<any>(null);
const pendingOperationExecution = ref<(() => void) | null>(null);






const menuItems = ref([
  { separator: true },
  {
    label: "Manual",
    icon: "pi pi-fw pi-wrench",
    command: () => {
      selectedMenu.value = MenuType.manual;
    },
  },
  {
    label: "Canned Cycles",
    icon: "pi pi-fw pi-cog",
    command: () => {
      selectedMenu.value = MenuType.cannedCycles;
    },
  },
  {
    label: "HAL",
    icon: "pi pi-fw pi-link",
    command: () => {
      selectedMenu.value = MenuType.halStatus;
    },
  },
  {
    label: "Settings",
    icon: "pi pi-fw pi-sliders-v",
    command: () => {
      selectedMenu.value = MenuType.settings;
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
    case EntryType.xPosition:
    numberentry.value = numbersPrevious = metric.value ? (diameterMode.value ? value * 2 : value) : (diameterMode.value ? value * 2 : value) / 25.4;
    break;
    case EntryType.zPosition:
    case EntryType.xPitch:
    case EntryType.zPitch:
    case ThreadingEntryType.threadPitch:
    case ThreadingEntryType.threadXDepth:
    case ThreadingEntryType.threadZDepth:
    case ThreadingEntryType.threadAngle:
    case ThreadingEntryType.threadZEnd:
    case ThreadingEntryType.threadXPullout:
    case ThreadingEntryType.threadZPullout:
    case ThreadingEntryType.threadFirstCut:
    case ThreadingEntryType.threadMinCut:
    numberentry.value = numbersPrevious = metric.value ? value : value / 25.4;
    break;
    case EntryType.aPosition:
    case ThreadingEntryType.threadCutMult:
    case ThreadingEntryType.threadSpringCuts:
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
    case EntryType.xPosition:
    case EntryType.zPosition:
    case EntryType.xPitch:
    case EntryType.zPitch:
    case ThreadingEntryType.threadPitch:
    case ThreadingEntryType.threadXDepth:
    case ThreadingEntryType.threadZDepth:
    case ThreadingEntryType.threadAngle:
    case ThreadingEntryType.threadZEnd:
    case ThreadingEntryType.threadXPullout:
    case ThreadingEntryType.threadZPullout:
    case ThreadingEntryType.threadFirstCut:
    case ThreadingEntryType.threadMinCut:
    if (!metric.value) {
      value = value * 25.4;
    }
    break;
    case EntryType.aPosition:
    case ThreadingEntryType.threadCutMult:
    case ThreadingEntryType.threadSpringCuts:
    break;
  }
  switch (entryActive.value) {
    case EntryType.xPosition:
      const actualXValue = diameterMode.value ? value / 2 : value;
      setAxisValue('x', actualXValue);
      xpos.value = actualXValue;
      break;
    case EntryType.zPosition:
      setAxisValue('z', value);
      zpos.value = value;
      break;
    case EntryType.aPosition:
      setAxisValue('a', value);
      apos.value = value;
      break;
    case EntryType.xPitch:
      xpitch.value = Math.abs(value);
      break;
    case EntryType.zPitch:
      zpitch.value = Math.abs(value);
      break;
    case ThreadingEntryType.threadPitch:
      setThreadingParameter(ThreadingEntryType.threadPitch, value);
      updatePitchFromThread();
      break;
    case ThreadingEntryType.threadXDepth:
      setThreadingParameter(ThreadingEntryType.threadXDepth, value);
      break;
    case ThreadingEntryType.threadZDepth:
      setThreadingParameter(ThreadingEntryType.threadZDepth, value);
      break;
    case ThreadingEntryType.threadAngle:
      setThreadingParameter(ThreadingEntryType.threadAngle, value);
      break;
    case ThreadingEntryType.threadZEnd:
      setThreadingParameter(ThreadingEntryType.threadZEnd, value);
      break;
    case ThreadingEntryType.threadXPullout:
      setThreadingParameter(ThreadingEntryType.threadXPullout, value);
      break;
    case ThreadingEntryType.threadZPullout:
      setThreadingParameter(ThreadingEntryType.threadZPullout, value);
      break;
    case ThreadingEntryType.threadFirstCut:
      setThreadingParameter(ThreadingEntryType.threadFirstCut, value);
      break;
    case ThreadingEntryType.threadCutMult:
      setThreadingParameter(ThreadingEntryType.threadCutMult, value);
      break;
    case ThreadingEntryType.threadMinCut:
      setThreadingParameter(ThreadingEntryType.threadMinCut, value);
      break;
    case ThreadingEntryType.threadSpringCuts:
      setThreadingParameter(ThreadingEntryType.threadSpringCuts, value);
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
          case ThreadingEntryType.threadPitch:
            clearThreadingParameter(ThreadingEntryType.threadPitch);
            break;
          case ThreadingEntryType.threadXDepth:
            clearThreadingParameter(ThreadingEntryType.threadXDepth);
            break;
          case ThreadingEntryType.threadZDepth:
            clearThreadingParameter(ThreadingEntryType.threadZDepth);
            break;
          case ThreadingEntryType.threadAngle:
            clearThreadingParameter(ThreadingEntryType.threadAngle);
            break;
          case ThreadingEntryType.threadZEnd:
            clearThreadingParameter(ThreadingEntryType.threadZEnd);
            break;
          case ThreadingEntryType.threadXPullout:
            clearThreadingParameter(ThreadingEntryType.threadXPullout);
            break;
          case ThreadingEntryType.threadZPullout:
            clearThreadingParameter(ThreadingEntryType.threadZPullout);
            break;
          case ThreadingEntryType.threadFirstCut:
            clearThreadingParameter(ThreadingEntryType.threadFirstCut);
            break;
          case ThreadingEntryType.threadCutMult:
            clearThreadingParameter(ThreadingEntryType.threadCutMult);
            break;
          case ThreadingEntryType.threadMinCut:
            clearThreadingParameter(ThreadingEntryType.threadMinCut);
            break;
          case ThreadingEntryType.threadSpringCuts:
            clearThreadingParameter(ThreadingEntryType.threadSpringCuts);
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
    case EntryType.xPosition:
      setAxisValue('x', 0);
      scheduleHALOut();
      break;
    case EntryType.zPosition:
      setAxisValue('z', 0);
      scheduleHALOut();
      break;
    case EntryType.aPosition:
      setAxisValue('a', 0);
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
    setAxisOffset('x', 0);
    setAxisOffset('z', 0);
  }
};

const stopHAL = () => {
  halStdoutText.value = "";
  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(" electron/") > -1) {
    window.api.send("stopHAL");
    xpos.value = 0;
    zpos.value = 0;
    setAxisOffset('x', 0);
    setAxisOffset('z', 0);
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


// Generic operation preview system
const showOperationPreviewDialog = (operationData: any, executeCallback: () => void) => {
  currentOperation.value = operationData;
  pendingOperationExecution.value = executeCallback;
  showOperationPreview.value = true;
};

const onPreviewContinue = () => {
  showOperationPreview.value = false;
  if (pendingOperationExecution.value) {
    pendingOperationExecution.value();
    pendingOperationExecution.value = null;
  }
  currentOperation.value = null;
};

const onPreviewCancel = () => {
  showOperationPreview.value = false;
  pendingOperationExecution.value = null;
  currentOperation.value = null;
};


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
  setButtonTime('up', Date.now() / 1000);
};

const touchEndUp = () => {
  scheduleButtonUp();
  stopJogNow();
};

const touchStartLeft = () => {
  setButtonTime('left', Date.now() / 1000);
};

const touchEndLeft = () => {
  scheduleButtonUp();
  stopJogNow();
};

const touchStartRight = () => {
  setButtonTime('right', Date.now() / 1000);
};

const touchEndRight = () => {
  stopJogNow();
  scheduleButtonUp();
};

const touchStartDown = () => {
  setButtonTime('down', Date.now() / 1000);
};

const touchEndDown = () => {
  scheduleButtonUp();
  stopJogNow();
};

const touchStop = () => {
  scheduleButtonUp();
  stopJogNow();
};



watch([selectedFeedMode, selectedDirectionMode], () => {
  updateHALOut(selectedFeedMode, selectedDirectionMode, FeedMode, DirectionMode);
});

watch([zpitch, xpitch], () => {
  updateHALOut(selectedFeedMode, selectedDirectionMode, FeedMode, DirectionMode);
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


const threadStartClicked = async () => {
  treatOffClickAsEnter();
  entryActive.value = 0;
  
  // Validate and execute threading using composable
  const isValid = validateThreadingParameters();
  if (!isValid) {
    return;
  }
  
  // Get HAL positions and generate threading operation
  try {
    const halIn = await getHalIn();
    const currentXPos = (halIn as any).position_x - getAxisOffset('x');
    const currentZPos = (halIn as any).position_z - getAxisOffset('z');
    const currentAPos = (halIn as any).position_a - getAxisOffset('a');
    const params = generateThreadingParams(currentXPos, currentZPos, currentAPos);
    
    // Generate G-code for preview
    const result = await generateThreadingGcode(params);
    if (result && result.gcode) {
      // Create G-code string for backplot
      const gcodeString = result.gcode.join('\n');
      const gcodeBase64 = btoa(gcodeString);
      
      // Send to backplot generator
      const backplotData = await putLinuxCNC("backplot", { gcode: gcodeBase64 });
      if (backplotData) {
        // Create operation data for preview
        const operationData = {
          name: "Threading Operation",
          type: "threading",
          parameters: {
            Pitch: threadPitch.value,
            XDepth: threadXDepth.value,
            ZDepth: threadZDepth.value,
            ZEnd: threadZEnd.value,
            Angle: threadAngle.value,
            XPullout: threadXPullout.value,
            ZPullout: threadZPullout.value,
            FirstCut: threadFirstCut.value,
            CutMult: threadCutMult.value,
            MinCut: threadMinCut.value,
            SpringCuts: threadSpringCuts.value
          },
          gcode: result.gcode,
          backplotData: backplotData
        };
        
        // Show preview dialog with callback to execute threading
        const putThreadingCallback = () => {
          putThreading(params);
        };
        
        showOperationPreviewDialog(operationData, putThreadingCallback);
      }
    }
  } catch (error) {
    console.error("Threading operation failed:", error);
    alert("Failed to start threading operation. Please check parameters and try again.");
  }
};

const threadStopClicked = () => {
  treatOffClickAsEnter();
  entryActive.value = 0;
  
  
  // Abort current operation (gentler than emergency stop)
  putAbort();
  cleanupCannedCycles();
};

const threadResetClicked = () => {
  treatOffClickAsEnter();
  entryActive.value = 0;
  
  // Hide backplot
  showBackplot.value = false;
  
  // Reset all threading parameters using composable function
  resetThreadingParameters();
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
  
  // If we have Z depth (compound threading), calculate cross feed
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
  
  // Convert threading values using composable function
  convertThreadingParameters(metric.value);
  updatePitchFromThread();
};

onMounted(async () => {
  // Load settings first
  await loadSettings();
  
  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(" electron/") > -1) {
    window.api.receive("halStarted", () => {
      selectedMenu.value = MenuType.manual;
      startPoll({
        selectedMenu,
        MenuType,
        DirectionMode,
        FeedMode,
        selectedDirectionMode,
        selectedFeedMode
      });
      updateHALOut(selectedFeedMode, selectedDirectionMode, FeedMode, DirectionMode);
    });

    window.api.receive("halStopped", () => {
      endPoll();
      cleanupCannedCycles();
    });

    window.api.receive("halStdout", (event: any, arg: any) => {
      halStdoutText.value += event as string;
    });

    selectedMenu.value = MenuType.halStatus;
    startHAL();
  } else {
    startPoll({
      selectedMenu,
      MenuType,
      DirectionMode,
      FeedMode,
      selectedDirectionMode,
      selectedFeedMode
    });
    updateHALOut(selectedFeedMode, selectedDirectionMode, FeedMode, DirectionMode);
  }
});

onUnmounted(() => {
  cleanupCannedCycles();
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
              <div :class="['stack-segment', 'segment-white', { 'active': machineStatus === 'cannedCycle' }]"></div>
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
    <div v-if="selectedMenu == MenuType.manual" class="m-2">
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
      v-if="selectedMenu == MenuType.cannedCycles"
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
          <div class="col-12 align-content-center">Canned Cycles</div>
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
            Threading{{ threadPresetName ? ` - ${threadPresetName}` : '' }}
          </div>
          <!-- Note: X Start uses current position, Z Start is always 0 -->
          
          <!-- Row 1 -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'P')">P</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(ThreadingEntryType.threadPitch, threadPitch || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != ThreadingEntryType.threadPitch && threadPitch === null }]"
              :style="{ backgroundColor: entryActive == ThreadingEntryType.threadPitch ? '#666' : '#333' }"
              :title="entryActive == ThreadingEntryType.threadPitch ? String(numberentry) : String(threadPitch ?? 'Pitch')"
            >
              {{ entryActive == ThreadingEntryType.threadPitch ? numberentry : (threadPitch ?? 'Pitch') }}
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
              @click="numberClicked(ThreadingEntryType.threadZDepth, threadZDepth || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != ThreadingEntryType.threadZDepth && threadZDepth === null }]"
              :style="{ backgroundColor: entryActive == ThreadingEntryType.threadZDepth ? '#666' : '#333' }"
              :title="entryActive == ThreadingEntryType.threadZDepth ? String(numberentry) : String(threadZDepth ?? 'Z Depth')"
            >
              {{ entryActive == ThreadingEntryType.threadZDepth ? numberentry : (threadZDepth ?? 'Z Depth') }}
            </button>
          </div>
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'XD')">XD</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(ThreadingEntryType.threadXDepth, threadXDepth || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != ThreadingEntryType.threadXDepth && threadXDepth === null }]"
              :style="{ backgroundColor: entryActive == ThreadingEntryType.threadXDepth ? '#666' : '#333' }"
              :title="entryActive == ThreadingEntryType.threadXDepth ? String(numberentry) : String(threadXDepth ?? 'X Depth')"
            >
              {{ entryActive == ThreadingEntryType.threadXDepth ? numberentry : (threadXDepth ?? 'X Depth') }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          <!-- Row 3 -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'ZE')">ZE</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(ThreadingEntryType.threadZEnd, threadZEnd || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != ThreadingEntryType.threadZEnd && threadZEnd === null }]"
              :style="{ backgroundColor: entryActive == ThreadingEntryType.threadZEnd ? '#666' : '#333' }"
              :title="entryActive == ThreadingEntryType.threadZEnd ? String(numberentry) : String(threadZEnd ?? 'Z End')"
            >
              {{ entryActive == ThreadingEntryType.threadZEnd ? numberentry : (threadZEnd ?? 'Z End') }}
            </button>
          </div>
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'A')">A</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(ThreadingEntryType.threadAngle, threadAngle || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != ThreadingEntryType.threadAngle && threadAngle === null }]"
              :style="{ backgroundColor: entryActive == ThreadingEntryType.threadAngle ? '#666' : '#333' }"
              :title="entryActive == ThreadingEntryType.threadAngle ? String(numberentry) : String(threadAngle ?? 'Angle')"
            >
              {{ entryActive == ThreadingEntryType.threadAngle ? numberentry : (threadAngle ?? 'Angle') }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          <!-- Row 4 -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'ZP')">ZP</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(ThreadingEntryType.threadZPullout, threadZPullout || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != ThreadingEntryType.threadZPullout && threadZPullout === null }]"
              :style="{ backgroundColor: entryActive == ThreadingEntryType.threadZPullout ? '#666' : '#333' }"
              :title="entryActive == ThreadingEntryType.threadZPullout ? String(numberentry) : String(threadZPullout ?? 'Z Pullout')"
            >
              {{ entryActive == ThreadingEntryType.threadZPullout ? numberentry : (threadZPullout ?? 'Z Pullout') }}
            </button>
          </div>
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'XP')">XP</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(ThreadingEntryType.threadXPullout, threadXPullout || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != ThreadingEntryType.threadXPullout && threadXPullout === null }]"
              :style="{ backgroundColor: entryActive == ThreadingEntryType.threadXPullout ? '#666' : '#333' }"
              :title="entryActive == ThreadingEntryType.threadXPullout ? String(numberentry) : String(threadXPullout ?? 'X Pullout')"
            >
              {{ entryActive == ThreadingEntryType.threadXPullout ? numberentry : (threadXPullout ?? 'X Pullout') }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          <!-- Row 5 -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'CM')">CM</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(ThreadingEntryType.threadCutMult, threadCutMult || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != ThreadingEntryType.threadCutMult && threadCutMult === null }]"
              :style="{ backgroundColor: entryActive == ThreadingEntryType.threadCutMult ? '#666' : '#333' }"
              :title="entryActive == ThreadingEntryType.threadCutMult ? String(numberentry) : String(threadCutMult ?? 'Cut Multiplier')"
            >
              {{ entryActive == ThreadingEntryType.threadCutMult ? numberentry : (threadCutMult ?? 'Cut Multiplier') }}
            </button>
          </div>
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'FC')">FC</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(ThreadingEntryType.threadFirstCut, threadFirstCut || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != ThreadingEntryType.threadFirstCut && threadFirstCut === null }]"
              :style="{ backgroundColor: entryActive == ThreadingEntryType.threadFirstCut ? '#666' : '#333' }"
              :title="entryActive == ThreadingEntryType.threadFirstCut ? String(numberentry) : String(threadFirstCut ?? 'First Cut')"
            >
              {{ entryActive == ThreadingEntryType.threadFirstCut ? numberentry : (threadFirstCut ?? 'First Cut') }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          <!-- Row 6 -->
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'SC')">SC</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(ThreadingEntryType.threadSpringCuts, threadSpringCuts || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != ThreadingEntryType.threadSpringCuts && threadSpringCuts === null }]"
              :style="{ backgroundColor: entryActive == ThreadingEntryType.threadSpringCuts ? '#666' : '#333' }"
              :title="entryActive == ThreadingEntryType.threadSpringCuts ? String(numberentry) : String(threadSpringCuts ?? 'Spring Cuts')"
            >
              {{ entryActive == ThreadingEntryType.threadSpringCuts ? numberentry : (threadSpringCuts ?? 'Spring Cuts') }}
            </button>
          </div>
          <div class="col-1 text-right p-0 flex align-items-center justify-content-end cursor-pointer" @click="showLabelPopover($event, 'MC')">MC</div>
          <div class="col-4 p-1">
            <button
              @click="numberClicked(ThreadingEntryType.threadMinCut, threadMinCut || 0)"
              :class="['w-full text-left dro-font-mode button-mode p-1 truncate', { 'placeholder-text': entryActive != ThreadingEntryType.threadMinCut && threadMinCut === null }]"
              :style="{ backgroundColor: entryActive == ThreadingEntryType.threadMinCut ? '#666' : '#333' }"
              :title="entryActive == ThreadingEntryType.threadMinCut ? String(numberentry) : String(threadMinCut ?? 'Min Cut')"
            >
              {{ entryActive == ThreadingEntryType.threadMinCut ? numberentry : (threadMinCut ?? 'Min Cut') }}
            </button>
          </div>
          <div class="col-2 p-0"></div>
          
          
          
          </div>
          
          <!-- Spacer to push buttons to bottom -->
          <div class="flex-grow-1"></div>
          
          <!-- Start, Stop, Preset and Reset buttons centered as a group -->
          <div class="flex justify-content-center gap-2 p-1">
            <button
              @click="openThreadPresetDialog(metric, updatePitchFromThread)"
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
      
      <!-- Operation Preview Dialog -->
      <Dialog 
        v-model:visible="showOperationPreview" 
        modal 
        :showHeader="false"
        :draggable="false"
        :closable="false"
        :style="{ width: '90vw', height: '90vh' }"
        :pt="{ content: { style: 'padding: 0; height: 100%;' } }"
      >
        <OperationPreview
          v-if="currentOperation"
          :operation="currentOperation"
          @continue="onPreviewContinue"
          @cancel="onPreviewCancel"
        />
      </Dialog>
    </div>
    <div
      v-if="selectedMenu == MenuType.halStatus"
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
      v-if="selectedMenu == MenuType.settings"
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
 