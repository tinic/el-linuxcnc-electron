<script setup lang="ts">
import { computed, ref } from 'vue'

import AppSidebar from './components/AppSidebar.vue'
import CyclePanel from './components/CyclePanel.vue'
import DroPanel from './components/DroPanel.vue'
import LogPage from './components/LogPage.vue'
import ManualPanel from './components/ManualPanel.vue'
import ModalDialog from './components/ModalDialog.vue'
import NumPad from './components/NumPad.vue'
import PresetPicker from './components/PresetPicker.vue'
import SettingsPage from './components/SettingsPage.vue'
import ToolTable from './components/ToolTable.vue'
import { settle } from './entry'
import { dismissNotice, panel, setPitch, setXTaper, updatePreferences } from './machine'
import { pitchSections, type PitchPreset } from './presets'

const pitchAxis = ref<'x' | 'z' | null>(null)
const showTools = ref(false)

const machinePage = computed(() => panel.page === 'manual' || panel.page === 'cycles')

const pitchTab = computed(() => {
  const tabs = panel.settings?.pitch_preset_tabs ?? [0, 0]
  return pitchAxis.value === 'x' ? tabs[0] : tabs[1]
})

function selectPitchTab(index: number): void {
  const tabs = panel.settings?.pitch_preset_tabs ?? [0, 0]
  updatePreferences({
    pitch_preset_tabs: pitchAxis.value === 'x' ? [index, tabs[1]] : [tabs[0], index],
  })
}

function pickPitch(preset: PitchPreset): void {
  if (pitchAxis.value === 'x' && preset.type === 'angle') {
    setXTaper(preset.value, preset.name)
  } else if (pitchAxis.value !== null) {
    setPitch(pitchAxis.value, preset.value, preset.name)
  }
  pitchAxis.value = null
}

function closeTools(): void {
  settle()
  showTools.value = false
}
</script>

<template>
  <div class="shell">
    <AppSidebar />
    <main v-if="machinePage" class="machine">
      <div class="top">
        <DroPanel
          :show-pitches="panel.page === 'manual'"
          @pitch-presets="pitchAxis = $event"
          @tool-table="showTools = true"
        />
        <NumPad class="pad" />
      </div>
      <div class="bottom">
        <ManualPanel v-if="panel.page === 'manual'" />
        <CyclePanel v-else />
      </div>
    </main>
    <main v-else-if="panel.page === 'log'" class="page"><LogPage /></main>
    <main v-else class="page"><SettingsPage /></main>

    <ModalDialog
      v-if="pitchAxis"
      :title="pitchAxis === 'x' ? 'Cross feed (PX)' : 'Longitudinal feed (PZ)'"
      @close="pitchAxis = null"
    >
      <PresetPicker
        :sections="pitchSections(pitchAxis)"
        :tab="pitchTab"
        @tab="selectPitchTab"
        @pick="pickPitch"
      />
    </ModalDialog>

    <ModalDialog v-if="showTools" title="Tool table" wide @close="closeTools">
      <ToolTable @close="closeTools" />
    </ModalDialog>

    <div v-if="panel.notice" class="notice" :class="panel.notice.level" @click="dismissNotice">
      {{ panel.notice.text }}
    </div>
  </div>
</template>

<style scoped>
.shell {
  display: flex;
  width: 100%;
  height: 100%;
}

main {
  flex: 1;
  min-width: 0;
  height: 100%;
}

.machine {
  display: grid;
  grid-template-rows: auto 1fr;
  padding: 14px 18px 18px;
  gap: 14px;
}

.top {
  display: flex;
  justify-content: space-between;
  gap: 20px;
  padding-bottom: 14px;
  border-bottom: 1px solid var(--line);
}

.pad {
  padding-left: 20px;
  border-left: 1px solid var(--line);
}

.bottom {
  min-height: 0;
}

.notice {
  position: fixed;
  left: 50%;
  bottom: 20px;
  transform: translateX(-50%);
  max-width: 70vw;
  padding: 14px 20px;
  border-radius: var(--radius);
  background: #2b2b2b;
  border: 1px solid var(--line);
  box-shadow: 0 8px 24px rgb(0 0 0 / 50%);
}

.notice.error {
  border-color: var(--stop);
  color: #fecaca;
}
</style>
