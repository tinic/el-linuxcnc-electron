<script setup lang="ts">
import { ref, computed } from "vue";

const emit = defineEmits([
  "numberClicked",
  "zeroClicked",
  "pitchClicked",
  "otherClicked",
  "metricClicked",
]);

enum NumberEntry {
  none = 0,
  xpos = 1,
  zpos = 2,
  apos = 3,
  xpitch = 4,
  zpitch = 5,
}

interface Props {
  xpos: number;
  zpos: number;
  apos: number;
  rpms: number;
  xpitch: number;
  zpitch: number;
  xlock: boolean;
  zlock: boolean;
  xpitchactive: boolean;
  zpitchactive: boolean;
  xpitchlabel: string;
  zpitchlabel: string;
  numberentry: number;
  entryActive: number;
  metric: boolean;
  cursorpos: number;
}

const props = withDefaults(defineProps<Props>(), {
  xpos: 1.1223,
  zpos: -22.34452,
  apos: 275.32323323,
  rpms: 5433.23432432,
  xpitch: 0.4,
  zpitch: 0.4,
  xlock: true,
  zlock: false,
  xpitchactive: false,
  zpitchactive: true,
  xpitchlabel: "...",
  zpitchlabel: "...",
  numberentry: 0,
  entryActive: 0,
  metric: true,
  cursorpos: 0,
});

enum ZeroEntry {
  none = 0,
  xpos0 = 1,
  zpos0 = 2,
  apos0 = 3,
}

const numberTotalLength: number = 10;

const xposFormatted = computed(() => {
  let xpos = props.xpos;
  if (props.entryActive == NumberEntry.xpos) {
    xpos = props.numberentry;
  } else {
    xpos = props.metric ? xpos : xpos / 25.4;
  }
  let xposStr = xpos.toFixed(props.metric ? 3 : 4);
  return " ".repeat(numberTotalLength - xposStr.length) + xposStr;
});

const xposUnitFormatted = computed(() => {
  return (props.metric ? "mm" : "″ ") + " ".repeat(4);
});

const xposCursorFormatted = computed(() => {
  if (props.entryActive == NumberEntry.xpos) {
    return " ".repeat(props.cursorpos + (props.metric ? 8 : 7)) + "_";
  } else {
    return "";
  }
});

const zposFormatted = computed(() => {
  let zpos = props.zpos;
  if (props.entryActive == NumberEntry.zpos) {
    zpos = props.numberentry;
  } else {
    zpos = props.metric ? zpos : zpos / 25.4;
  }
  let zposStr = zpos.toFixed(props.metric ? 3 : 4);
  return " ".repeat(numberTotalLength - zposStr.length) + zposStr;
});

const zposUnitFormatted = computed(() => {
  return (props.metric ? "mm" : "″ ") + " ".repeat(4);
});

const zposCursorFormatted = computed(() => {
  if (props.entryActive == NumberEntry.zpos) {
    return " ".repeat(props.cursorpos + (props.metric ? 8 : 7)) + "_";
  } else {
    return "";
  }
});

const aposFormatted = computed(() => {
  let apos = props.apos;
  if (props.entryActive == NumberEntry.apos) {
    apos = props.numberentry;
  } else {
    if (apos >= 0) {
      apos %= 360;
    } else {
      apos = 360 + (apos % 360);
    }
  }
  let aposStr = apos.toFixed(3);
  return " ".repeat(numberTotalLength - aposStr.length) + aposStr;
});

const aposUnitFormatted = computed(() => {
  return "°" + " ".repeat(5);
});

const aposCursorFormatted = computed(() => {
  if (props.entryActive == NumberEntry.apos) {
    return " ".repeat(props.cursorpos + 8) + "_";
  } else {
    return "";
  }
});

const rpmsFormatted = computed(() => {
  let rpmsStr = props.rpms?.toFixed(3);
  return " ".repeat(numberTotalLength - rpmsStr.length) + rpmsStr;
});

const xpitchFormatted = computed(() => {
  let xpitch = props.xpitch;
  if (props.entryActive == NumberEntry.xpitch) {
    xpitch = props.numberentry;
  } else {
    xpitch = props.metric ? xpitch : xpitch / 25.4;
  }
  let xpitchStr = xpitch.toFixed(props.metric ? 3 : 4);
  return " ".repeat(numberTotalLength - xpitchStr.length) + xpitchStr;
});

const xpitchUnitFormatted = computed(() => {
  return props.metric ? "mm/rev" : "″/rev ";
});

const xpitchCursorFormatted = computed(() => {
  if (props.entryActive == NumberEntry.xpitch) {
    return " ".repeat(props.cursorpos + (props.metric ? 8 : 7)) + "_";
  } else {
    return "";
  }
});

const zpitchFormatted = computed(() => {
  let zpitch = props.zpitch;
  if (props.entryActive == NumberEntry.zpitch) {
    zpitch = props.numberentry;
  } else {
    zpitch = props.metric ? zpitch : zpitch / 25.4;
  }
  let zpitchStr = zpitch.toFixed(props.metric ? 3 : 4);
  return " ".repeat(numberTotalLength - zpitchStr.length) + zpitchStr;
});

const zpitchUnitFormatted = computed(() => {
  return props.metric ? "mm/rev" : "″/rev ";
});

const zpitchCursorFormatted = computed(() => {
  if (props.entryActive == NumberEntry.zpitch) {
    return " ".repeat(props.cursorpos + (props.metric ? 8 : 7)) + "_";
  } else {
    return "";
  }
});


const rpmsUnitFormatted = computed(() => {
  return "rpm" + " ".repeat(3);
});

const xposClicked = () => {
  emit("numberClicked", NumberEntry.xpos, props.xpos);
};

const xpos0Clicked = () => {
  emit("zeroClicked", ZeroEntry.xpos0);
};

const zposClicked = () => {
  emit("numberClicked", NumberEntry.zpos, props.zpos);
};

const zpos0Clicked = () => {
  emit("zeroClicked", ZeroEntry.zpos0);
};

const aposClicked = () => {
  emit("numberClicked", NumberEntry.apos, props.apos);
};

const apos0Clicked = () => {
  emit("zeroClicked", ZeroEntry.apos0);
};

const xpitchClicked = () => {
  emit("numberClicked", NumberEntry.xpitch, props.xpitch);
};

const zpitchClicked = () => {
  emit("numberClicked", NumberEntry.zpitch, props.zpitch);
};

const xpitchSelectClicked = () => {
  emit("pitchClicked", "x");
};

const zpitchSelectClicked = () => {
  emit("pitchClicked", "z");
};

const rpmClicked = () => {
  emit("otherClicked");
};

const unitClicked = () => {
  emit("metricClicked");
};

const xposLabel = computed(() => {
  return props.entryActive == NumberEntry.xpos
    ? '<span style="color:#ff0000"> X|</span>'
    : '<span style="color:#aaaaaa"> X|</span>';
});

const zposLabel = computed(() => {
  return props.entryActive == NumberEntry.zpos
    ? '<span style="color:#ff0000"> Z|</span>'
    : '<span style="color:#aaaaaa"> Z|</span>';
});

const aposLabel = computed(() => {
  return props.entryActive == NumberEntry.apos
    ? '<span style="color:#ff0000"> A|</span>'
    : '<span style="color:#aaaaaa"> A|</span>';
});

const xpitchLabel = computed(() => {
  return props.entryActive == NumberEntry.xpitch
    ? '<span style="color:#ff0000">PX|</span>'
    : '<span style="color:#aaaaaa">PX|</span>';
});

const zpitchLabel = computed(() => {
  return props.entryActive == NumberEntry.zpitch
    ? '<span style="color:#ff0000">PZ|</span>'
    : '<span style="color:#aaaaaa">PZ|</span>';
});
</script>

<template>
  <div class="bg-gray-900 inline dro-font-display p-2 keep-spaces">
    <div @click="xposClicked" class="inline">
      <span v-html="xposLabel" />{{ xposFormatted }}<font size="-1">&nbsp;</font
><font color="#aaaaaa">{{ xposUnitFormatted }}</font>
    </div>
    <button
      @click="xpos0Clicked"
      class="dro-font-display-button align-content-center ml-5"
      style="width: 6em; padding: 0.75rem"
    >
      X₀
    </button>
    <i
      class="pi pi-lock ml-4"
      style="color: #ff0000; font-size: 1.5rem"
      v-if="props.xlock"
    />
    <i
      class="pi pi-lock ml-4"
      style="color: #000000; font-size: 1.5rem"
      v-else
    />
    <i
      class="pi pi-cog ml-4"
      style="color: #ff0000; font-size: 1.5rem"
      v-if="props.xpitchactive"
    />
    <i
      class="pi pi-cog ml-4"
      style="color: #000000; font-size: 1.5rem"
      v-else
    />
    <br />
    <div @click="zposClicked" class="inline">
      <span v-html="zposLabel" />{{ zposFormatted }}<font size="-1">&nbsp;</font
      ><font color="#aaaaaa">{{ zposUnitFormatted }}</font>
    </div>
    <button
      @click="zpos0Clicked"
      class="dro-font-display-button align-content-center ml-5"
      style="width: 6em; padding: 0.75rem"
    >
      Z₀
    </button>
    <i
      class="pi pi-lock ml-4"
      style="color: #ff0000; font-size: 1.5rem"
      v-if="props.zlock"
    />
    <i
      class="pi pi-lock ml-4"
      style="color: #000000; font-size: 1.5rem"
      v-else
    />
    <i
      class="pi pi-cog ml-4"
      style="color: #ff0000; font-size: 1.5rem"
      v-if="props.zpitchactive"
    />
    <i
      class="pi pi-cog ml-4"
      style="color: #000000; font-size: 1.5rem"
      v-else
    />
    <br />
    <div @click="aposClicked" class="inline">
      <span v-html="aposLabel" />{{ aposFormatted }}<font size="-1">&nbsp;</font
      ><font color="#aaaaaa">{{ aposUnitFormatted }}</font>
    </div>
    <button
      @click="apos0Clicked"
      class="dro-font-display-button align-content-center ml-5"
      style="width: 6em; padding: 0.75rem"
    >
      A₀
    </button>
    <br />
    <div @click="xpitchClicked" class="inline">
      <span v-html="xpitchLabel" />{{ xpitchFormatted
      }}<font size="-1">&nbsp;</font
      ><font color="#aaaaaa">{{ xpitchUnitFormatted }}</font>
    </div>
    <button
      @click="xpitchSelectClicked"
      class="dro-font-display-button align-content-center ml-5"
      style="width: 6em; padding: 0.75rem"
    >
      {{ props.xpitchlabel }}
    </button>
    <br />
    <div @click="zpitchClicked" class="inline">
      <span v-html="zpitchLabel" />{{ zpitchFormatted
      }}<font size="-1">&nbsp;</font
      ><font color="#aaaaaa">{{ zpitchUnitFormatted }}</font>
    </div>
    <button
      @click="zpitchSelectClicked"
      class="dro-font-display-button align-content-center ml-5"
      style="width: 6em; padding: 0.75rem"
    >
      {{ props.zpitchlabel }}
    </button>
    <br />
    <div @click="rpmClicked" class="inline">
      <font color="#aaaaaa">&nbsp;R|</font>{{ rpmsFormatted
      }}<font size="-1">&nbsp;</font
      ><font color="#aaaaaa">{{ rpmsUnitFormatted }}</font>
    </div>
    <button
      @click="unitClicked"
      class="dro-font-display-button align-content-center ml-5"
      style="width: 6em; padding: 0.75rem"
    >
      mm↔in
    </button>
    <br />
    <div style="position: absolute; top: 0.71em">
      <span style="color:#ff0000">{{xposCursorFormatted}}</span>
    </div>
    <div style="position: absolute; top: 2.40em">
      <span style="color:#ff0000">{{zposCursorFormatted}}</span>
    </div>
    <div style="position: absolute; top: 4.05em">
      <span style="color:#ff0000">{{aposCursorFormatted}}</span>
    </div>
    <div style="position: absolute; top: 5.72em">
      <span style="color:#ff0000">{{xpitchCursorFormatted}}</span>
    </div>
    <div style="position: absolute; top: 7.40em">
      <span style="color:#ff0000">{{zpitchCursorFormatted}}</span>
    </div>
  </div>
</template>

<style>
.keep-spaces {
  white-space: pre-wrap;
}

.dro-font-display-button {
  background: #333;
  text-align: center;
}

.dro-font-display {
  font-family: "iosevka";
  font-weight: bold;
  font-size: 2.1em;
  text-align: left;
}

.dro-font-display-button {
  font-family: "iosevka";
  font-weight: bold;
  font-size: 0.6em;
  text-align: center;
}
</style>
