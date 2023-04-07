
<script setup lang="ts">
import { ref, computed } from 'vue';

export interface Props {
    xpos:number,
    zpos:number,
    apos:number,
    rpms:number,
    xpitch:number,
    zpitch:number,
    xlock:boolean,
    zlock:boolean,
    xpitchactive:boolean,
    zpitchactive:boolean,
};

const props = withDefaults(defineProps<Props>(), {
    xpos: 1.1223,
    zpos: -22.34452,
    apos: 275.32323323,
    rpms: 5433.23432432,
    xpitch: 0.4,
    zpitch: 0.4,
    xlock: true,
    zlock: false,
    xpitchactive:false,
    zpitchactive:true
});

const metric = ref(true);

const unitClicked = () => {
    metric.value = !metric.value;
};

const xposFormatted = computed(() => {
    let xpos = props.xpos;
    xpos = metric.value ? xpos : xpos / 25.4;
    let xposStr = xpos.toFixed(metric.value ? 3 : 4);
    return " ".repeat(10 - xposStr.length) + xposStr;
});

const xposUnitFormatted = computed(() => {
    return (metric.value ? 'mm' : '\" ') + " ".repeat(3);
});

const zposFormatted = computed(() => {
    let zpos = props.zpos;
    zpos = metric.value ? zpos : zpos / 25.4;
    let zposStr = zpos.toFixed(metric.value ? 3 : 4);
    return " ".repeat(10 - zposStr.length) + zposStr;
});

const zposUnitFormatted = computed(() => {
    return (metric.value ? 'mm' : '\" ') + " ".repeat(3);
});

const aposFormatted = computed(() => {
    let apos = props.apos;
    if (apos > 0) {
        apos %= 360;
    } else {
        apos  = (360 + apos % 360)
    }
    let aposStr = apos.toFixed(3);
    return " ".repeat(10 - aposStr.length) + aposStr;
});

const aposUnitFormatted = computed(() => {
    return "°" + " ".repeat(4);
});

const rpmsFormatted = computed(() => {
    let rpmsStr = props.rpms?.toFixed(3);
    return " ".repeat(10 - rpmsStr.length) + rpmsStr;
});

const rpmsUnitFormatted = computed(() => {
    return "rpm" + " ".repeat(2);
});

const xpitchFormatted = computed(() => {
    let xpitch = props.xpitch;
    xpitch = metric.value ? xpitch : xpitch / 25.4;
    let xpitchStr = xpitch.toFixed(metric.value ? 3 : 4);
    return " ".repeat(10 - xpitchStr.length) + xpitchStr;
});

const xpitchUnitFormatted = computed(() => {
    return (metric.value ? 'mm/rev' : '\"/rev');
});

const zpitchFormatted = computed(() => {
    let zpitch = props.zpitch;
    zpitch = metric.value ? zpitch : zpitch / 25.4;
    let zpitchStr = zpitch.toFixed(metric.value ? 3 : 4);
    return " ".repeat(10 - zpitchStr.length) + zpitchStr;
});

const zpitchUnitFormatted = computed(() => {
    return (metric.value ? 'mm/rev' : '\"/rev');
});


</script>

<template>  
    <div class="bg-gray-900 dro-font-display p-3 keep-spaces">
        <div>
            <font color='#aaaaaa'>&nbsp;X|</font>{{ xposFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ xposUnitFormatted }}</font>
            <i class='pi pi-lock' style='color: #ff0000; font-size: 1.5rem' v-if='props.xlock'/>
            <i class='pi pi-lock' style='color: #000000; font-size: 1.5rem' v-else/>
            <button class="dro-font-display-button align-content-center ml-5" style="width:4.5em; padding: 0.75rem;">X₀</button>
        </div>
        <div>
            <font color='#aaaaaa'>&nbsp;Z|</font>{{ zposFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ zposUnitFormatted }}</font>
            <i class='pi pi-lock' style='color: #ff0000; font-size: 1.5rem' v-if='props.zlock'/>
            <i class='pi pi-lock' style='color: #000000; font-size: 1.5rem' v-else/>
            <button class="dro-font-display-button align-content-center ml-5" style="width:4.5em; padding: 0.75rem;">Z₀</button>
        </div>
        <div>
            <font color='#aaaaaa'>&nbsp;A|</font>{{ aposFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ aposUnitFormatted }}</font>
            <i class='pi pi-lock' style='color: #111111; font-size: 1.5rem'/>
            <button class="dro-font-display-button align-content-center ml-5" style="width:4.5em; padding: 0.75rem;">A₀</button>
        </div>
        <div>
            <font color='#aaaaaa'>PX|</font>{{ xpitchFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ xpitchUnitFormatted }}</font>
        </div>
        <div>
            <font color='#aaaaaa'>PZ|</font>{{ zpitchFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ zpitchUnitFormatted }}</font>
        </div>
        <div>
            <font color='#aaaaaa'>&nbsp;R|</font>{{ rpmsFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ rpmsUnitFormatted }}</font>
            <i class='pi pi-lock' style='color: #111111; font-size: 1.5rem'/>
            <button @click="unitClicked" class="dro-font-display-button align-content-center ml-5" style="width:4.5em; padding: 0.75rem;">mm↔in</button>
        </div>
    </div>
</template>

<style>
.keep-spaces { white-space: pre-wrap; }

.dro-font-display-button {
    background: #333;
    text-align: center;
}

.dro-font-display {
  font-family: 'iosevka';
  font-weight: bold;
  font-size: 2.25em;
  text-align: left;
}

.dro-font-display-button {
  font-family: 'iosevka';
  font-weight: bold;
  font-size: 0.6em;
  text-align: center;

}
</style>
