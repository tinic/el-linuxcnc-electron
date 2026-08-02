<template>
  <div class="flex-grow-1 flex flex-column">
    <Toolbar class="flex-none p-1">
      <template #start>
        <Button
          label="Start HAL"
          icon="pi pi-play"
          class="mr-2"
          @click="$emit('start')"
        />
        <Button
          label="Stop HAL"
          icon="pi pi-stop"
          severity="success"
          @click="$emit('stop')"
        />
      </template>
    </Toolbar>
    <Textarea
      :model-value="output"
      auto-scroll="true"
      class="console-output flex-grow-1"
      spellcheck="false"
      autocomplete="off"
      autocorrect="off"
      autocapitalize="off"
    />
  </div>
</template>

<script setup lang="ts">
defineProps<{
  output: string;
}>()

defineEmits<{
  start: [];
  stop: [];
}>()
</script>

<style scoped>
.console-output {
  font-family: 'iosevka' !important;
  font-size: 1em;
  line-height: 1.2;
  height: 100% !important;
  resize: none;
}
</style>
