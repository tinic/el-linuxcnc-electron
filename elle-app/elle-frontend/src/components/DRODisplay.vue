<script setup lang="ts">
import { computed } from 'vue'

const emit = defineEmits([
  'numberClicked',
  'zeroClicked',
  'pitchClicked',
  'otherClicked',
  'metricClicked'
])

enum NumberEntry {
  // eslint-disable-next-line no-unused-vars
  xpos = 1,
  // eslint-disable-next-line no-unused-vars
  zpos = 2,
  // eslint-disable-next-line no-unused-vars
  apos = 3,
  // eslint-disable-next-line no-unused-vars
  xpitch = 4,
  // eslint-disable-next-line no-unused-vars
  zpitch = 5,
}

interface Props {
  xpos?: number
  zpos?: number
  apos?: number
  rpms?: number
  xpitch?: number
  zpitch?: number
  xlock?: boolean
  zlock?: boolean
  xpitchactive?: boolean
  zpitchactive?: boolean
  xpitchlabel?: string
  zpitchlabel?: string
  numberentry?: number
  entryActive?: number
  metric?: boolean
  cursorpos?: number
  diameterMode?: boolean
  showXPitch?: boolean
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
  xpitchlabel: '...',
  zpitchlabel: '...',
  numberentry: 0,
  entryActive: 0,
  metric: true,
  cursorpos: 0,
  diameterMode: false,
  showXPitch: true
})

enum ZeroEntry {
  // eslint-disable-next-line no-unused-vars
  xpos0 = 1,
  // eslint-disable-next-line no-unused-vars
  zpos0 = 2,
  // eslint-disable-next-line no-unused-vars
  apos0 = 3,
}

const numberTotalLength: number = 10

const xposFormatted = computed(() => {
  let xpos = props.xpos
  if (props.entryActive == NumberEntry.xpos) {
    xpos = props.numberentry
  } else {
    xpos = props.metric ? xpos : xpos / 25.4
  }
  const xposStr = xpos.toFixed(props.metric ? 3 : 4)
  return ' '.repeat(numberTotalLength - xposStr.length) + xposStr
})

const xposUnitFormatted = computed(() => (props.metric ? 'mm' : '″ ') + ' '.repeat(4))

const xposCursorFormatted = computed(() => {
  if (props.entryActive == NumberEntry.xpos) {
    return `${' '.repeat(props.cursorpos + (props.metric ? 8 : 7))  }_`
  } else {
    return ''
  }
})

const zposFormatted = computed(() => {
  let zpos = props.zpos
  if (props.entryActive == NumberEntry.zpos) {
    zpos = props.numberentry
  } else {
    zpos = props.metric ? zpos : zpos / 25.4
  }
  const zposStr = zpos.toFixed(props.metric ? 3 : 4)
  return ' '.repeat(numberTotalLength - zposStr.length) + zposStr
})

const zposUnitFormatted = computed(() => (props.metric ? 'mm' : '″ ') + ' '.repeat(4))

const zposCursorFormatted = computed(() => {
  if (props.entryActive == NumberEntry.zpos) {
    return `${' '.repeat(props.cursorpos + (props.metric ? 8 : 7))  }_`
  } else {
    return ''
  }
})

const aposFormatted = computed(() => {
  let apos = props.apos
  if (props.entryActive == NumberEntry.apos) {
    apos = props.numberentry
  } else {
    if (apos >= 0) {
      apos %= 360
    } else {
      apos = 360 + (apos % 360)
    }
  }
  const aposStr = apos.toFixed(3)
  return ' '.repeat(numberTotalLength - aposStr.length) + aposStr
})

const aposUnitFormatted = computed(() => `°${  ' '.repeat(5)}`)

const aposCursorFormatted = computed(() => {
  if (props.entryActive == NumberEntry.apos) {
    return `${' '.repeat(props.cursorpos + 8)  }_`
  } else {
    return ''
  }
})

const rpmsFormatted = computed(() => {
  const rpmsStr = props.rpms?.toFixed(3)
  return ' '.repeat(numberTotalLength - rpmsStr.length) + rpmsStr
})

const xpitchFormatted = computed(() => {
  let xpitch = props.xpitch
  if (props.entryActive == NumberEntry.xpitch) {
    xpitch = props.numberentry
  } else {
    xpitch = props.metric ? xpitch : xpitch / 25.4
  }
  const xpitchStr = xpitch.toFixed(props.metric ? 3 : 4)
  return ' '.repeat(numberTotalLength - xpitchStr.length) + xpitchStr
})

const xpitchUnitFormatted = computed(() => props.metric ? 'mm/rev' : '″/rev ')

const xpitchCursorFormatted = computed(() => {
  if (props.entryActive == NumberEntry.xpitch) {
    return `${' '.repeat(props.cursorpos + (props.metric ? 8 : 7))  }_`
  } else {
    return ''
  }
})

const zpitchFormatted = computed(() => {
  let zpitch = props.zpitch
  if (props.entryActive == NumberEntry.zpitch) {
    zpitch = props.numberentry
  } else {
    zpitch = props.metric ? zpitch : zpitch / 25.4
  }
  const zpitchStr = zpitch.toFixed(props.metric ? 3 : 4)
  return ' '.repeat(numberTotalLength - zpitchStr.length) + zpitchStr
})

const zpitchUnitFormatted = computed(() => props.metric ? 'mm/rev' : '″/rev ')

const zpitchCursorFormatted = computed(() => {
  if (props.entryActive == NumberEntry.zpitch) {
    return `${' '.repeat(props.cursorpos + (props.metric ? 8 : 7))  }_`
  } else {
    return ''
  }
})

const rpmsUnitFormatted = computed(() => `rpm${  ' '.repeat(3)}`)

const xposClicked = () => {
  emit('numberClicked', NumberEntry.xpos, props.xpos)
}

const xpos0Clicked = () => {
  emit('zeroClicked', ZeroEntry.xpos0)
}

const zposClicked = () => {
  emit('numberClicked', NumberEntry.zpos, props.zpos)
}

const zpos0Clicked = () => {
  emit('zeroClicked', ZeroEntry.zpos0)
}

const aposClicked = () => {
  emit('numberClicked', NumberEntry.apos, props.apos)
}

const apos0Clicked = () => {
  emit('zeroClicked', ZeroEntry.apos0)
}

const xpitchClicked = () => {
  emit('numberClicked', NumberEntry.xpitch, props.xpitch)
}

const zpitchClicked = () => {
  emit('numberClicked', NumberEntry.zpitch, props.zpitch)
}

const xpitchSelectClicked = () => {
  emit('pitchClicked', 'x')
}

const zpitchSelectClicked = () => {
  emit('pitchClicked', 'z')
}

const rpmClicked = () => {
  emit('otherClicked')
}

const unitClicked = () => {
  emit('metricClicked')
}

const xposLabel = computed(() => {
  const prefix = props.diameterMode ? '⌽' : ' '
  return props.entryActive == NumberEntry.xpos
    ? `<span style="color:#ff0000">${prefix}X|</span>`
    : `<span style="color:#aaaaaa">${prefix}X|</span>`
})

const zposLabel = computed(() => props.entryActive == NumberEntry.zpos
  ? '<span style="color:#ff0000"> Z|</span>'
  : '<span style="color:#aaaaaa"> Z|</span>')

const aposLabel = computed(() => props.entryActive == NumberEntry.apos
  ? '<span style="color:#ff0000"> A|</span>'
  : '<span style="color:#aaaaaa"> A|</span>')

const xpitchLabel = computed(() => props.entryActive == NumberEntry.xpitch
  ? '<span style="color:#ff0000">PX|</span>'
  : '<span style="color:#aaaaaa">PX|</span>')

const zpitchLabel = computed(() => props.entryActive == NumberEntry.zpitch
  ? '<span style="color:#ff0000">PZ|</span>'
  : '<span style="color:#aaaaaa">PZ|</span>')
</script>

<template>
  <div class="inline dro-font-display p-2 keep-spaces">
    <div class="inline" @click="xposClicked">
      <span v-html="xposLabel" />{{ xposFormatted }}<font size="-1">&nbsp;</font><font color="#aaaaaa">{{ xposUnitFormatted }}</font>
    </div>
    <button
      class="dro-font-display-button align-content-center ml-5"
      style="width: 6em; padding: 0.75rem"
      @click="xpos0Clicked"
    >
      X₀
    </button>
    <i v-if="props.xlock" class="pi pi-lock ml-4" style="color: #ff0000; font-size: 1.5rem" />
    <i v-else class="pi pi-lock ml-4" style="color: #000000; font-size: 1.5rem" />
    <i v-if="props.xpitchactive" class="pi pi-cog ml-4" style="color: #ff0000; font-size: 1.5rem" />
    <i v-else class="pi pi-cog ml-4" style="color: #000000; font-size: 1.5rem" />
    <br />
    <div class="inline" @click="zposClicked">
      <span v-html="zposLabel" />{{ zposFormatted }}<font size="-1">&nbsp;</font><font color="#aaaaaa">{{ zposUnitFormatted }}</font>
    </div>
    <button
      class="dro-font-display-button align-content-center ml-5"
      style="width: 6em; padding: 0.75rem"
      @click="zpos0Clicked"
    >
      Z₀
    </button>
    <i v-if="props.zlock" class="pi pi-lock ml-4" style="color: #ff0000; font-size: 1.5rem" />
    <i v-else class="pi pi-lock ml-4" style="color: #000000; font-size: 1.5rem" />
    <i v-if="props.zpitchactive" class="pi pi-cog ml-4" style="color: #ff0000; font-size: 1.5rem" />
    <i v-else class="pi pi-cog ml-4" style="color: #000000; font-size: 1.5rem" />
    <br />
    <div class="inline" @click="aposClicked">
      <span v-html="aposLabel" />{{ aposFormatted }}<font size="-1">&nbsp;</font><font color="#aaaaaa">{{ aposUnitFormatted }}</font>
    </div>
    <button
      class="dro-font-display-button align-content-center ml-5"
      style="width: 6em; padding: 0.75rem"
      @click="apos0Clicked"
    >
      A₀
    </button>
    <br />
    <div
      class="inline"
      :style="{ visibility: showXPitch ? 'visible' : 'hidden' }"
      @click="xpitchClicked"
    >
      <span v-html="xpitchLabel" />{{ xpitchFormatted }}<font size="-1">&nbsp;</font><font color="#aaaaaa">{{ xpitchUnitFormatted }}</font>
    </div>
    <button
      class="dro-font-display-button align-content-center ml-5"
      style="width: 6em; padding: 0.75rem"
      :style="{ visibility: showXPitch ? 'visible' : 'hidden' }"
      @click="xpitchSelectClicked"
    >
      {{ props.xpitchlabel }}
    </button>
    <br />
    <div class="inline" @click="zpitchClicked">
      <span v-html="zpitchLabel" />{{ zpitchFormatted }}<font size="-1">&nbsp;</font><font color="#aaaaaa">{{ zpitchUnitFormatted }}</font>
    </div>
    <button
      class="dro-font-display-button align-content-center ml-5"
      style="width: 6em; padding: 0.75rem"
      @click="zpitchSelectClicked"
    >
      {{ props.zpitchlabel }}
    </button>
    <br />
    <div class="inline" @click="rpmClicked">
      <font color="#aaaaaa">&nbsp;R|</font>{{ rpmsFormatted }}<font size="-1">&nbsp;</font><font color="#aaaaaa">{{ rpmsUnitFormatted }}</font>
    </div>
    <button
      class="dro-font-display-button align-content-center ml-5"
      style="width: 6em; padding: 0.75rem"
      @click="unitClicked"
    >
      mm↔in
    </button>
    <br />
    <div style="position: absolute; top: 0.71em">
      <span style="color: #ff0000">{{ xposCursorFormatted }}</span>
    </div>
    <div style="position: absolute; top: 2.4em">
      <span style="color: #ff0000">{{ zposCursorFormatted }}</span>
    </div>
    <div style="position: absolute; top: 4.05em">
      <span style="color: #ff0000">{{ aposCursorFormatted }}</span>
    </div>
    <div style="position: absolute; top: 5.72em">
      <span style="color: #ff0000">{{ xpitchCursorFormatted }}</span>
    </div>
    <div style="position: absolute; top: 7.4em">
      <span style="color: #ff0000">{{ zpitchCursorFormatted }}</span>
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
  font-family: 'iosevka';
  font-weight: bold;
  font-size: 2.1em;
  text-align: left;
}

.dro-font-display-button {
  font-family: 'iosevka';
  font-weight: bold;
  font-size: 0.6em;
  text-align: center;
}
</style>
