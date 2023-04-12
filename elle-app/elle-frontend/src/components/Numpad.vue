<script setup lang="ts">
import { emit } from 'process';
const emit = defineEmits(['numPadClicked']);
const numPadClicked = (key:string) => {
    emit('numPadClicked', key);
}
</script>

<template>  
    <div class="bg-gray-900 align-content-center">
    <div class="grid dro-font-numpad grid-nogutter p-1" style="width:350px">
        <div class="col-3 p-1"><button @click="numPadClicked('Escape')" class="w-full button-numpad p-3">ESC</button></div>
        <div class="col-3 p-1"><button @click="numPadClicked('Third')" class="w-full button-numpad p-3">⅓</button></div>
        <div class="col-3 p-1"><button @click="numPadClicked('Half')" class="w-full button-numpad p-3">½</button></div>
        <div class="col-3 p-1"><button @click="numPadClicked('Backspace')" class="w-full button-numpad p-3">⌫</button></div>
        <div class="col-9">
            <div class="grid grid-nogutter">
                <div class="col-4 p-1"><button @click="numPadClicked('7')" class="w-full button-numpad p-3">7</button></div>
                <div class="col-4 p-1"><button @click="numPadClicked('8')" class="w-full button-numpad p-3">8</button></div>
                <div class="col-4 p-1"><button @click="numPadClicked('9')" class="w-full button-numpad p-3">9</button></div>
                <div class="col-4 p-1"><button @click="numPadClicked('4')" class="w-full button-numpad p-3">4</button></div>
                <div class="col-4 p-1"><button @click="numPadClicked('5')" class="w-full button-numpad p-3">5</button></div>
                <div class="col-4 p-1"><button @click="numPadClicked('6')" class="w-full button-numpad p-3">6</button></div>
            </div>
        </div>
        <div class="col-3 p-1"><button  @click="numPadClicked('PlusMinus')" class="w-full h-full button-numpad p-3">±</button></div>
        <div class="col-9">
            <div class="grid grid-nogutter">
                <div class="col-4 p-1"><button @click="numPadClicked('1')" class="w-full button-numpad p-3">1</button></div>
                <div class="col-4 p-1"><button @click="numPadClicked('2')" class="w-full button-numpad p-3">2</button></div>
                <div class="col-4 p-1"><button @click="numPadClicked('3')" class="w-full button-numpad p-3">3</button></div>
                <div class="col-8 p-1"><button @click="numPadClicked('0')" class="w-full button-numpad p-3">0</button></div>
                <div class="col-4 p-1"><button @click="numPadClicked('.')" class="w-full button-numpad p-3">.</button></div>
            </div>
        </div>
        <div class="col-3 p-1"><button @click="numPadClicked('Enter')" class="w-full h-full button-numpad p-3">ENT</button></div>
    </div>
    </div>
</template>

<style>
.button-numpad {
    background: #333;
}

.dro-font-numpad {
  font-family: 'iosevka';
  font-weight: bold;
  font-size: 1.3em;
  text-align: center;
}
</style>
