<script setup lang="ts">
import { onMounted, useTemplateRef } from 'vue'

defineProps<{ title: string; wide?: boolean }>()
const emit = defineEmits<{ close: [] }>()

const dialog = useTemplateRef<HTMLDialogElement>('dialog')

onMounted(() => {
  const element = dialog.value
  element?.showModal()
  // Focus the dialog itself so no control shows a focus ring on a touch screen.
  element?.focus()
})
</script>

<template>
  <dialog ref="dialog" :class="{ wide }" tabindex="-1" @cancel.prevent="emit('close')">
    <header>
      <h2>{{ title }}</h2>
      <button class="close" aria-label="Close" @click="emit('close')">✕</button>
    </header>
    <div class="body">
      <slot />
    </div>
  </dialog>
</template>

<style scoped>
dialog {
  width: min(900px, 92vw);
  max-height: 92vh;
  padding: 0;
  border: 1px solid var(--line);
  border-radius: 12px;
  color: var(--text);
  background: var(--panel);
  display: flex;
  flex-direction: column;
}

dialog.wide {
  width: 94vw;
  height: 92vh;
}

dialog:focus {
  outline: none;
}

dialog::backdrop {
  background: rgb(0 0 0 / 60%);
}

header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 10px 10px 10px 20px;
  border-bottom: 1px solid var(--line);
}

h2 {
  margin: 0;
  font-size: 1.1rem;
}

.close {
  width: 48px;
  height: 48px;
  font-size: 1.2rem;
}

.body {
  /* Content-sized, growing to fill a fixed-height (wide) dialog and scrolling within max-height.
     Not `flex: 1`: its 0% basis collapses the body to nothing in WebKit when the dialog has no
     fixed height (Chromium sizes it to content instead). */
  flex: 1 1 auto;
  min-height: 0;
  overflow: auto;
  padding: 16px 20px 20px;
}
</style>
