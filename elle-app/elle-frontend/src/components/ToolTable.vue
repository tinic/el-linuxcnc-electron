<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import Dialog from 'primevue/dialog'
import DataTable from 'primevue/datatable'
import Column from 'primevue/column'
import InputText from 'primevue/inputtext'
import InputNumber from 'primevue/inputnumber'
import { useToolTable } from '../composables/useToolTable'
import { useSettings } from '../composables/useSettings'

interface Props {
  visible: boolean
  currentToolId: number
}

const props = defineProps<Props>()

const emit = defineEmits<{
  'update:visible': [value: boolean]
  'tool-selected': [toolId: number, offsetX: number, offsetZ: number]
}>()

const { tools, selectTool, updateTool, formatOffset, parseOffset } = useToolTable()
const { metric } = useSettings()

// Local state for editing
const editingId = ref<number | null>(null)
const editingField = ref<string | null>(null)
const editingValue = ref<string>('')

// Set initial selection
watch(() => props.currentToolId, (newId) => {
  selectTool(newId)
}, { immediate: true })

const unitLabel = computed(() => metric.value ? 'mm' : 'in')

const closeDialog = () => {
  emit('update:visible', false)
}

const onRowClick = (event: any) => {
  const tool = event.data
  selectTool(tool.id)
  const offsetX = tool.offsetX
  const offsetZ = tool.offsetZ
  emit('tool-selected', tool.id, offsetX, offsetZ)
}

const startEdit = (tool: any, field: string) => {
  editingId.value = tool.id
  editingField.value = field
  
  if (field === 'offsetX') {
    editingValue.value = formatOffset(tool.offsetX)
  } else if (field === 'offsetZ') {
    editingValue.value = formatOffset(tool.offsetZ)
  } else if (field === 'description') {
    editingValue.value = tool.description
  }
}

const finishEdit = () => {
  if (editingId.value !== null && editingField.value) {
    const tool = tools.value.find(t => t.id === editingId.value)
    if (tool) {
      if (editingField.value === 'offsetX') {
        const offsetX = parseOffset(editingValue.value)
        updateTool(tool.id, offsetX, tool.offsetZ, tool.description)
      } else if (editingField.value === 'offsetZ') {
        const offsetZ = parseOffset(editingValue.value)
        updateTool(tool.id, tool.offsetX, offsetZ, tool.description)
      } else if (editingField.value === 'description') {
        updateTool(tool.id, tool.offsetX, tool.offsetZ, editingValue.value)
      }
    }
  }
  
  editingId.value = null
  editingField.value = null
  editingValue.value = ''
}

const cancelEdit = () => {
  editingId.value = null
  editingField.value = null
  editingValue.value = ''
}

const isRowSelected = (tool: any) => {
  return tool.id === props.currentToolId
}

const getRowClass = (tool: any) => {
  return isRowSelected(tool) ? 'selected-row' : ''
}
</script>

<template>
  <Dialog 
    :visible="props.visible" 
    @update:visible="closeDialog"
    modal
    header="Tool Table"
    :style="{ width: '90vw', maxWidth: '800px' }"
    :pt="{
      header: { class: 'p-3' },
      content: { class: 'p-0' }
    }"
  >

    <DataTable 
      :value="tools" 
      :row-class="getRowClass"
      class="tool-table"
      scrollable
      :scrollHeight="'60vh'"
    >
      <Column header="" :style="{ width: '60px', textAlign: 'center' }">
        <template #body="slotProps">
          <div @click="onRowClick({ data: slotProps.data })" class="clickable-cell">
            <i 
              v-if="isRowSelected(slotProps.data)" 
              class="pi pi-check-circle" 
              style="color: #22c55e; font-size: 1.5rem"
            />
            <i 
              v-else 
              class="pi pi-circle" 
              style="color: #666; font-size: 1.5rem"
            />
          </div>
        </template>
      </Column>

      <Column field="id" header="Tool ID" :style="{ width: '100px' }">
        <template #body="slotProps">
          <div @click="onRowClick({ data: slotProps.data })" class="clickable-cell">
            <span class="tool-id">T{{ slotProps.data.id.toString().padStart(2, '0') }}</span>
          </div>
        </template>
      </Column>

      <Column field="offsetX" :header="`X Offset (${unitLabel})`" :style="{ width: '150px' }">
        <template #body="slotProps">
          <div v-if="editingId === slotProps.data.id && editingField === 'offsetX'">
            <InputText 
              v-model="editingValue" 
              @keyup.enter="finishEdit"
              @keyup.escape="cancelEdit"
              @blur="finishEdit"
              class="w-full"
              type="number"
              :step="metric ? '0.001' : '0.0001'"
            />
          </div>
          <div 
            v-else 
            @click="startEdit(slotProps.data, 'offsetX')"
            class="editable-field"
          >
            {{ formatOffset(slotProps.data.offsetX) }}
          </div>
        </template>
      </Column>

      <Column field="offsetZ" :header="`Z Offset (${unitLabel})`" :style="{ width: '150px' }">
        <template #body="slotProps">
          <div v-if="editingId === slotProps.data.id && editingField === 'offsetZ'">
            <InputText 
              v-model="editingValue" 
              @keyup.enter="finishEdit"
              @keyup.escape="cancelEdit"
              @blur="finishEdit"
              class="w-full"
              type="number"
              :step="metric ? '0.001' : '0.0001'"
            />
          </div>
          <div 
            v-else 
            @click="startEdit(slotProps.data, 'offsetZ')"
            class="editable-field"
          >
            {{ formatOffset(slotProps.data.offsetZ) }}
          </div>
        </template>
      </Column>

      <Column field="description" header="Tool Description">
        <template #body="slotProps">
          <div v-if="editingId === slotProps.data.id && editingField === 'description'">
            <InputText 
              v-model="editingValue" 
              @keyup.enter="finishEdit"
              @keyup.escape="cancelEdit"
              @blur="finishEdit"
              class="w-full"
            />
          </div>
          <div 
            v-else 
            @click="startEdit(slotProps.data, 'description')"
            class="editable-field"
          >
            {{ slotProps.data.description || '—' }}
          </div>
        </template>
      </Column>
    </DataTable>
  </Dialog>
</template>

<style scoped>
.tool-table :deep(.p-datatable-tbody > tr) {
  font-size: 1.1rem;
}

.clickable-cell {
  cursor: pointer;
  padding: 0.33rem;
  border-radius: 4px;
  transition: background-color 0.2s;
}

.clickable-cell:hover {
  background-color: rgba(255, 255, 255, 0.05);
}

.tool-table :deep(.p-datatable-tbody > tr.selected-row) {
  background-color: rgba(34, 197, 94, 0.1);
}

.tool-table :deep(.p-datatable-header) {
  background-color: #1e1e1e;
  border-bottom: 1px solid #333;
}

.tool-table :deep(.p-datatable-thead > tr > th) {
  background-color: transparent;
  border: none;
  padding: 1rem;
  font-weight: 600;
  color: #aaa;
}

.tool-table :deep(.p-datatable-tbody > tr > td) {
  border: none;
  border-bottom: 1px solid #333;
  padding: 0.67rem;
}

.editable-field {
  padding: 0.5rem;
  border-radius: 4px;
  min-height: 2rem;
  display: flex;
  align-items: center;
}

.editable-field:hover {
  background-color: rgba(255, 255, 255, 0.05);
  cursor: text;
}

.tool-id {
  font-family: 'iosevka', monospace;
  font-weight: bold;
  font-size: 1.2rem;
}

:deep(.p-dialog-header) {
  background-color: #1e1e1e;
  border-bottom: 1px solid #333;
}

:deep(.p-dialog-content) {
  background-color: #0a0a0a;
}

:deep(.p-inputtext) {
  font-size: 1.1rem;
  padding: 0.5rem;
}
</style>