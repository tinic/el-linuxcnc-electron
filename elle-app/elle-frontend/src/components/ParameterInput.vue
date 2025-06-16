<script setup lang="ts">
import { computed } from 'vue'

interface Props {
  label: string
  labelKey: string
  entryType: number
  value: number | string | null
  placeholder: string
  entryActive: number
  numberEntry: number
  colSpan?: number // Allow different column spans for flexible layouts
  readonly?: boolean // For display-only fields (like diameter info)
  showPopover?: boolean // Whether to show info popover on label click
  variant?: 'default' | 'compact' | 'wide' // Different styling variants for different canned cycles
}

const props = withDefaults(defineProps<Props>(), {
  colSpan: 4,
  readonly: false,
  showPopover: true,
  variant: 'default',
})

const emit = defineEmits<{
  labelClick: [event: Event, labelKey: string]
  numberClick: [entryType: number, value: number]
}>()

const handleLabelClick = (event: Event) => {
  if (props.showPopover) {
    emit('labelClick', event, props.labelKey)
  }
}

const handleNumberClick = () => {
  if (!props.readonly) {
    const numericValue = typeof props.value === 'number' ? props.value : 0
    emit('numberClick', props.entryType, numericValue)
  }
}

const isActive = computed(() => props.entryActive === props.entryType)

const displayValue = computed(() => {
  if (isActive.value) {
    return props.numberEntry
  }
  return props.value ?? props.placeholder
})

// Compute dynamic classes based on variant
const labelClasses = computed(() => {
  const base = 'text-right p-0 flex align-items-center justify-content-end'
  const cursor = props.showPopover ? 'cursor-pointer' : ''

  switch (props.variant) {
    case 'compact':
      return `col-1 ${base} ${cursor} text-xs`
    case 'wide':
      return `col-2 ${base} ${cursor}`
    default:
      return `col-1 ${base} ${cursor}`
  }
})

const buttonClasses = computed(() => {
  const base = 'w-full text-left dro-font-mode button-mode p-1 truncate'
  const placeholder = !isActive.value && props.value === null ? 'placeholder-text' : ''
  return [base, placeholder].filter(Boolean)
})

const buttonStyle = computed(() => ({
  backgroundColor: isActive.value ? '#666' : '#333',
}))

const readonlyStyle = computed(() => ({
  backgroundColor: '#333',
  color: '#999',
  cursor: 'default',
}))
</script>

<template>
  <div :class="labelClasses" @click="handleLabelClick">
    {{ label }}
  </div>
  <div :class="`col-${colSpan} p-1`">
    <button
      v-if="!readonly"
      @click="handleNumberClick"
      :class="buttonClasses"
      :style="buttonStyle"
      :title="isActive ? String(numberEntry) : String(value ?? placeholder)"
    >
      {{ displayValue }}
    </button>
    <div v-else :class="buttonClasses" :style="readonlyStyle" :title="String(value ?? placeholder)">
      {{ displayValue }}
    </div>
  </div>
</template>

<style scoped>
.placeholder-text {
  color: #666;
  font-style: italic;
}

/* Variant-specific styling */
.text-xs {
  font-size: 0.75rem;
}
</style>
