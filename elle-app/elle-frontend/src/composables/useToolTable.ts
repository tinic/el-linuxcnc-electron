import { ref, computed } from 'vue'
import { useSettings } from './useSettings'

export interface Tool {
  id: number
  offset: number
  description: string
}

export function useToolTable() {
  const { metric, tools, currentToolIndex, currentToolOffset } = useSettings()

  const selectedToolId = ref(0)

  const updateTool = (id: number, offset: number, description: string) => {
    const tool = tools.value.find(t => t.id === id)
    if (tool) {
      tool.offset = offset
      tool.description = description
    }
  }

  const selectTool = (id: number) => {
    selectedToolId.value = id
  }

  const getToolById = (id: number) => {
    return tools.value.find(t => t.id === id)
  }

  const formatOffset = (offset: number) => {
    if (metric.value) {
      return offset.toFixed(3)
    } else {
      // Convert mm to inches
      return (offset / 25.4).toFixed(4)
    }
  }

  const parseOffset = (value: string) => {
    const parsed = parseFloat(value)
    if (isNaN(parsed)) return 0
    
    if (!metric.value) {
      // Convert inches to mm for internal storage
      return parsed * 25.4
    }
    return parsed
  }

  return {
    tools,
    selectedToolId,
    updateTool,
    selectTool,
    getToolById,
    formatOffset,
    parseOffset
  }
}