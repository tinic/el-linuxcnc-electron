<script setup lang="ts">
import { nextTick, useTemplateRef, watch } from 'vue'

import { panel } from '../machine'

const view = useTemplateRef<HTMLPreElement>('view')

// Stay at the bottom while new output arrives, unless the operator scrolled up to read.
watch(
  () => panel.log.length,
  async () => {
    const element = view.value
    const atBottom = !element || element.scrollHeight - element.scrollTop - element.clientHeight < 40
    await nextTick()
    if (atBottom && view.value) {
      view.value.scrollTop = view.value.scrollHeight
    }
  },
)
</script>

<template>
  <div class="log">
    <h1>LinuxCNC</h1>
    <pre ref="view">{{ panel.log.join('\n') }}</pre>
  </div>
</template>

<style scoped>
.log {
  display: flex;
  flex-direction: column;
  height: 100%;
  padding: 16px;
  gap: 12px;
}

h1 {
  margin: 0;
  font-size: 1.2rem;
}

pre {
  flex: 1;
  min-height: 0;
  margin: 0;
  overflow: auto;
  padding: 12px 14px;
  border-radius: var(--radius);
  background: var(--bg-sunken);
  font: 400 0.9rem/1.35 'Iosevka', ui-monospace, monospace;
  color: #d6d6d6;
  white-space: pre-wrap;
  user-select: text;
  -webkit-user-select: text;
}
</style>
