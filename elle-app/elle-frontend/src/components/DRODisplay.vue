
<script setup lang="ts">
import { computed } from 'vue';

export interface Props {
    xpos:number,
    zpos:number,
    apos:number,
    rpms:number,
    metric:boolean,
    xlock:boolean,
    zlock:boolean
};

const props = withDefaults(defineProps<Props>(), {
    xpos: 1.1223,
    zpos: -22.34452,
    apos: 275.32323323,
    rpms: 5433.23432432,
    metric: true,
    xlock: true,
    zlock: false
});

const xposFormatted = computed(() => {
    let xpos = props.xpos;
    xpos = props.metric ? xpos : xpos / 25.4;
    let xposStr = xpos.toFixed(props.metric ? 3 : 4);
    return " ".repeat(10 - xposStr.length) + xposStr;
});

const xposUnitFormatted = computed(() => {
    return (props.metric ? 'mm' : 'in') + " ".repeat(3);
});

const zposFormatted = computed(() => {
    let zpos = props.zpos;
    zpos = props.metric ? zpos : zpos / 25.4;
    let zposStr = zpos.toFixed(props.metric ? 3 : 4);
    return " ".repeat(10 - zposStr.length) + zposStr;
});

const zposUnitFormatted = computed(() => {
    return (props.metric ? 'mm' : 'in') + " ".repeat(3);
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
    return "deg" + " ".repeat(2);
});

const rpmsFormatted = computed(() => {
    let rpmsStr = props.rpms?.toFixed(3);
    return " ".repeat(10 - rpmsStr.length) + rpmsStr;
});

const rpmsUnitFormatted = computed(() => {
    return "rpm" + " ".repeat(2);
});

</script>

<template>  
    <div class="bg-gray-900 dro-font p-2 keep-spaces">
            <font color='#aaaaaa'>X|</font>{{ xposFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ xposUnitFormatted }}</font>
            <i class='pi pi-lock' style='color: #ff0000; font-size: 1.5rem' v-if='props.xlock'/>
            <i class='pi pi-lock' style='color: #000000; font-size: 1.5rem' v-else/>
        <br>
            <font color='#aaaaaa'>Z|</font>{{ zposFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ zposUnitFormatted }}</font>
            <i class='pi pi-lock' style='color: #ff0000; font-size: 1.5rem' v-if='props.zlock'/>
            <i class='pi pi-lock' style='color: #000000; font-size: 1.5rem' v-else/>
        <br>
            <font color='#aaaaaa'>A|</font>{{ aposFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ aposUnitFormatted }}</font>
        <br>
            <font color='#aaaaaa'>R|</font>{{ rpmsFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ rpmsUnitFormatted }}</font>
    </div>
</template>

<style>
.keep-spaces { white-space: pre-wrap; }

.dro-font {
  font-family: 'iosevka';
  font-weight: bold;
  font-size: 2.25em;
  text-align: left;
}
</style>