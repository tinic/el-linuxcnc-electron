
<script setup lang="ts">
import { emit } from 'process';
import { ref, computed } from 'vue';

const emit = defineEmits(['numberClicked', 'zeroClicked', 'pitchClicked']);

 interface Props {
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
    xpitchlabel:string,
    zpitchlabel:string,
    numberentry:number,
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
    zpitchactive:true,
    xpitchlabel:"...",
    zpitchlabel:"...",
    numberentry: 0
});

const metric = ref(true);

enum NumberEntry {
    none = 0,
    xpos = 1,
    zpos = 2,
    apos = 3,
    xpitch = 4,
    zpitch = 5,
};

enum ZeroEntry {
    none = 0,
    xpos0 = 1,
    zpos0 = 2,
    apos0 = 3,
};

const entryActive = ref(NumberEntry.none);

const numberTotalLength:number = 10;

const xposFormatted = computed(() => {
    let xpos = entryActive.value == NumberEntry.xpos ? props.numberentry : props.xpos;
    xpos = metric.value ? xpos : xpos / 25.4;
    let xposStr = xpos.toFixed(metric.value ? 3 : 4);
    return " ".repeat(numberTotalLength - xposStr.length) + xposStr;
});

const xposUnitFormatted = computed(() => {
    return (metric.value ? 'mm' : '″ ') + " ".repeat(4);
});

const zposFormatted = computed(() => {
    let zpos = entryActive.value == NumberEntry.zpos ? props.numberentry : props.zpos;
    zpos = metric.value ? zpos : zpos / 25.4;
    let zposStr = zpos.toFixed(metric.value ? 3 : 4);
    return " ".repeat(numberTotalLength - zposStr.length) + zposStr;
});

const zposUnitFormatted = computed(() => {
    return (metric.value ? 'mm' : '″ ') + " ".repeat(4);
});

const aposFormatted = computed(() => {
    let apos = entryActive.value == NumberEntry.apos ? props.numberentry : props.apos;
    if (apos >= 0) {
        apos %= 360;
    } else {
        apos  = (360 + apos % 360)
    }
    let aposStr = apos.toFixed(3);
    return " ".repeat(numberTotalLength - aposStr.length) + aposStr;
});

const aposUnitFormatted = computed(() => {
    return "°" + " ".repeat(5);
});

const rpmsFormatted = computed(() => {
    let rpmsStr = props.rpms?.toFixed(3);
    return " ".repeat(numberTotalLength - rpmsStr.length) + rpmsStr;
});

const xpitchFormatted = computed(() => {
    let xpitch = entryActive.value == NumberEntry.xpitch ? props.numberentry : props.xpitch;
    xpitch = metric.value ? xpitch : xpitch / 25.4;
    let xpitchStr = xpitch.toFixed(metric.value ? 3 : 4);
    return " ".repeat(numberTotalLength - xpitchStr.length) + xpitchStr;
});

const xpitchUnitFormatted = computed(() => {
    return (metric.value ? 'mm/rev' : '″/rev ');
});

const zpitchFormatted = computed(() => {
    let zpitch = entryActive.value == NumberEntry.zpitch ? props.numberentry : props.zpitch;
    zpitch = metric.value ? zpitch : zpitch / 25.4;
    let zpitchStr = zpitch.toFixed(metric.value ? 3 : 4);
    return " ".repeat(numberTotalLength - zpitchStr.length) + zpitchStr;
});

const zpitchUnitFormatted = computed(() => {
    return (metric.value ? 'mm/rev' : '″/rev ');
});

const rpmsUnitFormatted = computed(() => {
    return "rpm" + " ".repeat(3);
});

const xposClicked = () => {
    entryActive.value = NumberEntry.xpos;
    emit('numberClicked', NumberEntry.xpos)
}

const xpos0Clicked = () => {
    entryActive.value = NumberEntry.none;
    emit('zeroClicked', ZeroEntry.xpos0);
}

const zposClicked = () => {
    entryActive.value = NumberEntry.zpos;
    emit('numberClicked', NumberEntry.zpos)
}

const zpos0Clicked = () => {
    entryActive.value = NumberEntry.none;
    emit('zeroClicked', ZeroEntry.zpos0);
}

const aposClicked = () => {
    entryActive.value = NumberEntry.apos;
    emit('numberClicked', NumberEntry.apos)
}

const apos0Clicked = () => {
    entryActive.value = NumberEntry.none;
    emit('zeroClicked', ZeroEntry.apos0);
}

const xpitchClicked = () => {
    entryActive.value = NumberEntry.xpitch;
    emit('numberClicked', NumberEntry.xpitch)
}

const zpitchClicked = () => {
    entryActive.value = NumberEntry.zpitch;
    emit('numberClicked', NumberEntry.zpitch)
}

const xpitchSelectClicked = () => {
    entryActive.value = NumberEntry.none;
    emit('pitchClicked', 'x')
}

const zpitchSelectClicked = () => {
    entryActive.value = NumberEntry.none;
    emit('pitchClicked', 'z')
}

const rpmClicked = () => {
    entryActive.value = NumberEntry.none;
}

const unitClicked = () => {
    metric.value = !metric.value;
    entryActive.value = NumberEntry.none;
};

const xposLabel = computed(() => {
    return entryActive.value == NumberEntry.xpos ? '<span style="color:#ff0000"> X|</span>' : '<span style="color:#aaaaaa"> X|</span>';
});

const zposLabel = computed(() => {
    return entryActive.value == NumberEntry.zpos ? '<span style="color:#ff0000"> Z|</span>' : '<span style="color:#aaaaaa"> Z|</span>';
});

const aposLabel = computed(() => {
    return entryActive.value == NumberEntry.apos ? '<span style="color:#ff0000"> A|</span>' : '<span style="color:#aaaaaa"> A|</span>';
});

const xpitchLabel = computed(() => {
    return entryActive.value == NumberEntry.xpitch ? '<span style="color:#ff0000">PX|</span>' : '<span style="color:#aaaaaa">PX|</span>';
});

const zpitchLabel = computed(() => {
    return entryActive.value == NumberEntry.zpitch ? '<span style="color:#ff0000">PZ|</span>' : '<span style="color:#aaaaaa">PZ|</span>';
});

</script>

<template>  
    <div class="bg-gray-900 inline dro-font-display p-2 keep-spaces">
        <div @click="xposClicked" class="inline">
            <span v-html="xposLabel"/>{{ xposFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ xposUnitFormatted }}</font>
        </div>
        <button @click="xpos0Clicked" class="dro-font-display-button align-content-center ml-5" style="width:6em; padding: 0.75rem;">X₀</button>
        <i class='pi pi-lock ml-4' style='color: #ff0000; font-size: 1.5rem' v-if='props.xlock'/>
        <i class='pi pi-lock ml-4' style='color: #000000; font-size: 1.5rem' v-else/>
        <i class='pi pi-cog ml-4' style='color: #ff0000; font-size: 1.5rem' v-if='props.xpitchactive'/>
        <i class='pi pi-cog ml-4' style='color: #000000; font-size: 1.5rem' v-else/>
        <br/>
        <div @click="zposClicked" class="inline">
            <span v-html="zposLabel"/>{{ zposFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ zposUnitFormatted }}</font>
        </div>
        <button @click="zpos0Clicked" class="dro-font-display-button align-content-center ml-5" style="width:6em; padding: 0.75rem;">Z₀</button>
        <i class='pi pi-lock ml-4' style='color: #ff0000; font-size: 1.5rem' v-if='props.zlock'/>
        <i class='pi pi-lock ml-4' style='color: #000000; font-size: 1.5rem' v-else/>
        <i class='pi pi-cog ml-4' style='color: #ff0000; font-size: 1.5rem' v-if='props.zpitchactive'/>
        <i class='pi pi-cog ml-4' style='color: #000000; font-size: 1.5rem' v-else/>
        <br/>
        <div @click="aposClicked" class="inline">
            <span v-html="aposLabel"/>{{ aposFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ aposUnitFormatted }}</font>
        </div>
        <button @click="apos0Clicked" class="dro-font-display-button align-content-center ml-5" style="width:6em; padding: 0.75rem;">A₀</button>
        <br/>
        <div @click="xpitchClicked" class="inline">
            <span v-html="xpitchLabel"/>{{ xpitchFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ xpitchUnitFormatted }}</font>
        </div>
        <button @click="xpitchSelectClicked" class="dro-font-display-button align-content-center ml-5" style="width:6em; padding: 0.75rem;">{{ props.xpitchlabel }}</button>
        <br/>
        <div @click="zpitchClicked" class="inline">
            <span v-html="zpitchLabel"/>{{ zpitchFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ zpitchUnitFormatted }}</font>
        </div>
        <button @click="zpitchSelectClicked" class="dro-font-display-button align-content-center ml-5" style="width:6em; padding: 0.75rem;">{{ props.zpitchlabel }}</button>
        <br/>
        <div @click="rpmClicked" class="inline">
            <font color='#aaaaaa'>&nbsp;R|</font>{{ rpmsFormatted }}<font size="-1">&nbsp;</font><font color='#aaaaaa'>{{ rpmsUnitFormatted }}</font>
        </div>
        <button @click="unitClicked" class="dro-font-display-button align-content-center ml-5" style="width:6em; padding: 0.75rem;">mm↔in</button>
        <br/>
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
