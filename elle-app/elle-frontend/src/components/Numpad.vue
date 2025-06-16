<script setup lang="ts">
const emit = defineEmits(['numPadClicked'])
const numPadClicked = (key: string) => {
  emit('numPadClicked', key)
}
</script>

<template>
  <div class="align-content-center">
    <div class="grid dro-font-numpad grid-nogutter p-1" style="width: 350px">
      <div class="col-3 p-1">
        <button class="w-full button-numpad p-3" @click="numPadClicked('Escape')">ESC</button>
      </div>
      <div class="col-3 p-1">
        <button class="w-full button-numpad p-3" @click="numPadClicked('Third')">⅓</button>
      </div>
      <div class="col-3 p-1">
        <button class="w-full button-numpad p-3" @click="numPadClicked('Half')">½</button>
      </div>
      <div class="col-3 p-1">
        <button class="w-full button-numpad p-3" @click="numPadClicked('Backspace')">⌫</button>
      </div>
      <div class="col-9">
        <div class="grid grid-nogutter">
          <div class="col-4 p-1">
            <button class="w-full button-numpad p-3" @click="numPadClicked('7')">7</button>
          </div>
          <div class="col-4 p-1">
            <button class="w-full button-numpad p-3" @click="numPadClicked('8')">8</button>
          </div>
          <div class="col-4 p-1">
            <button class="w-full button-numpad p-3" @click="numPadClicked('9')">9</button>
          </div>
          <div class="col-4 p-1">
            <button class="w-full button-numpad p-3" @click="numPadClicked('4')">4</button>
          </div>
          <div class="col-4 p-1">
            <button class="w-full button-numpad p-3" @click="numPadClicked('5')">5</button>
          </div>
          <div class="col-4 p-1">
            <button class="w-full button-numpad p-3" @click="numPadClicked('6')">6</button>
          </div>
        </div>
      </div>
      <div class="col-3 p-1">
        <button class="w-full h-full button-numpad p-3" @click="numPadClicked('PlusMinus')">
          ±
        </button>
      </div>
      <div class="col-9">
        <div class="grid grid-nogutter">
          <div class="col-4 p-1">
            <button class="w-full button-numpad p-3" @click="numPadClicked('1')">1</button>
          </div>
          <div class="col-4 p-1">
            <button class="w-full button-numpad p-3" @click="numPadClicked('2')">2</button>
          </div>
          <div class="col-4 p-1">
            <button class="w-full button-numpad p-3" @click="numPadClicked('3')">3</button>
          </div>
          <div class="col-8 p-1">
            <button class="w-full button-numpad p-3" @click="numPadClicked('0')">0</button>
          </div>
          <div class="col-4 p-1">
            <button class="w-full button-numpad p-3" @click="numPadClicked('.')">.</button>
          </div>
        </div>
      </div>
      <div class="col-3 p-1">
        <button class="w-full h-full button-numpad p-3" @click="numPadClicked('Enter')">ENT</button>
      </div>
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
