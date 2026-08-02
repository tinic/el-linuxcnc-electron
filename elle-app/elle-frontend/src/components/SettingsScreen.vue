<template>
  <div class="flex-grow-1 p-4">
    <div class="dro-font-mode">
      <h2 class="mb-4">Settings</h2>

      <div class="grid grid-nogutter">
        <!-- Diameter Mode Setting -->
        <div class="col-12 mb-4">
          <div class="grid grid-nogutter align-items-center">
            <div class="col-6 text-right pr-4">
              <label class="text-lg font-semibold">Display Mode:</label>
            </div>
            <div class="col-6">
              <div class="flex gap-3">
                <button
                  :class="['button-mode p-2 px-4', { 'bg-primary': !diameterMode }]"
                  @click="diameterMode = false"
                >
                  Radius
                </button>
                <button
                  :class="['button-mode p-2 px-4', { 'bg-primary': diameterMode }]"
                  @click="diameterMode = true"
                >
                  Diameter
                </button>
              </div>
            </div>
          </div>
        </div>

        <!-- Default Units Setting -->
        <div class="col-12 mb-4">
          <div class="grid grid-nogutter align-items-center">
            <div class="col-6 text-right pr-4">
              <label class="text-lg font-semibold">Default Units:</label>
            </div>
            <div class="col-6">
              <div class="flex gap-3">
                <button
                  :class="['button-mode p-2 px-4', { 'bg-primary': defaultMetricOnStartup }]"
                  @click="defaultMetricOnStartup = true"
                >
                  Metric (mm)
                </button>
                <button
                  :class="['button-mode p-2 px-4', { 'bg-primary': !defaultMetricOnStartup }]"
                  @click="defaultMetricOnStartup = false"
                >
                  Imperial (inch)
                </button>
              </div>
            </div>
          </div>
        </div>

        <!-- Encoder Scale Settings -->
        <div class="col-12 mb-4">
          <div class="grid grid-nogutter align-items-center">
            <div class="col-6 text-right pr-4">
              <label class="text-lg font-semibold">Encoder Scale Z:</label>
            </div>
            <div class="col-6">
              <div class="flex gap-3">
                <InputNumber
                  v-model="encoderScaleZ"
                  :min="-1"
                  :max="1"
                  :step="0.0001"
                  :min-fraction-digits="4"
                  :max-fraction-digits="4"
                  class="w-8rem"
                />
              </div>
            </div>
          </div>
        </div>

        <div class="col-12 mb-4">
          <div class="grid grid-nogutter align-items-center">
            <div class="col-6 text-right pr-4">
              <label class="text-lg font-semibold">Encoder Scale X:</label>
            </div>
            <div class="col-6">
              <div class="flex gap-3">
                <InputNumber
                  v-model="encoderScaleX"
                  :min="-1"
                  :max="1"
                  :step="0.0001"
                  :min-fraction-digits="4"
                  :max-fraction-digits="4"
                  class="w-8rem"
                />
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import InputNumber from 'primevue/inputnumber'
import { useSettings } from '../composables/useSettings'

const { diameterMode, defaultMetricOnStartup, encoderScaleZ, encoderScaleX } = useSettings()
</script>

