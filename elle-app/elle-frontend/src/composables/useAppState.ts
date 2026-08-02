import { ref } from 'vue'

export enum MenuType {
  manual = 0,
  cannedCycles = 1,
  halStatus = 2,
  settings = 3
}

// Singleton: the active screen is shared app-wide (the HAL poll loop
// changes behavior based on it).
export const selectedMenu = ref(MenuType.manual)

export function useAppState() {
  return { MenuType, selectedMenu }
}
