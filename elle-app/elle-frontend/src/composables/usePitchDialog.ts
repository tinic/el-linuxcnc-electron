import { defineAsyncComponent } from 'vue'
import { useDialog } from 'primevue/usedialog'
import { useHAL, pitchForAngle } from './useHAL'
import { useNumpadEntry } from './useNumpadEntry'

const PitchPresetSelector = defineAsyncComponent(
  () => import('../components/PitchPresetSelector.vue')
)

const { xpitch, zpitch, xpitchlabel, zpitchlabel, xpitchangle } = useHAL()

// The PrimeVue dialog service is setup-scoped, so this composable must be
// called from a component's setup context.
export function usePitchDialog() {
  const dialog = useDialog()
  const { treatOffClickAsEnter, entryActive } = useNumpadEntry()

  const pitchClicked = (axis: string) => {
    treatOffClickAsEnter()
    entryActive.value = 0
    dialog.open(PitchPresetSelector, {
      props: {
        header: 'Select Pitch',
        style: {
          width: '70vw'
        },
        breakpoints: {
          '960px': '75vw',
          '640px': '90vw'
        },
        position: 'top',
        modal: true
      },
      data: {
        axis: axis
      },
      emits: {
        onSelected: (axis: string, name: string, value: number, type: string) => {
          switch (axis) {
            case 'z':
              if (type != 'angle') {
                zpitch.value = value
                zpitchlabel.value = name
                if (xpitchangle.value > 0) {
                  xpitch.value = pitchForAngle(zpitch.value, xpitchangle.value)
                }
              }
              break
            case 'x':
              if (type != 'angle') {
                xpitch.value = value
                xpitchlabel.value = name
                xpitchangle.value = 0
              } else {
                xpitch.value = pitchForAngle(zpitch.value, value)
                xpitchlabel.value = name
                xpitchangle.value = value
              }
              break
          }
        }
      },
      templates: {},
      onClose: (_options) => {}
    })
  }

  return { pitchClicked }
}
