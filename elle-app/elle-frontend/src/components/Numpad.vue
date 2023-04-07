<script setup lang="ts">
import { computed } from 'vue';
</script>

<template>  
    <div class="grid bg-gray-900 dro-font-numpad grid-nogutter p-1" style="width:350px">
        <div class="col-3 p-1"><button class="w-full button-numpad p-3">ESC</button></div>
        <div class="col-3 p-1"><button class="w-full button-numpad p-3">X</button></div>
        <div class="col-3 p-1"><button class="w-full button-numpad p-3">Z</button></div>
        <div class="col-3 p-1"><button class="w-full button-numpad p-3">A</button></div>
        <div class="col-9">
            <div class="grid grid-nogutter">
                <div class="col-4 p-1"><button class="w-full button-numpad p-3">7</button></div>
                <div class="col-4 p-1"><button class="w-full button-numpad p-3">8</button></div>
                <div class="col-4 p-1"><button class="w-full button-numpad p-3">9</button></div>
                <div class="col-4 p-1"><button class="w-full button-numpad p-3">4</button></div>
                <div class="col-4 p-1"><button class="w-full button-numpad p-3">5</button></div>
                <div class="col-4 p-1"><button class="w-full button-numpad p-3">6</button></div>
            </div>
        </div>
        <div class="col-3 p-1"><button class="w-full h-full button-numpad p-3">±</button></div>
        <div class="col-9">
            <div class="grid grid-nogutter">
                <div class="col-4 p-1"><button class="w-full button-numpad p-3">1</button></div>
                <div class="col-4 p-1"><button class="w-full button-numpad p-3">2</button></div>
                <div class="col-4 p-1"><button class="w-full button-numpad p-3">3</button></div>
                <div class="col-8 p-1"><button class="w-full button-numpad p-3">0</button></div>
                <div class="col-4 p-1"><button class="w-full button-numpad p-3">.</button></div>
            </div>
        </div>
        <div class="col-3 p-1"><button class="w-full h-full button-numpad p-3">ENT</button></div>
    </div>
</template>

<style>

.button-numpad {
    background: #333;
}

.dro-font-numpad {
  font-family: 'iosevka';
  font-weight: bold;
  font-size: 1.5em;
  text-align: center;
}
</style>
