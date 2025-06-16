import { createApp } from 'vue'
import './style.css'
import App from './App.vue'
import PrimeVue from 'primevue/config'
import { createWebHistory, createRouter } from 'vue-router'
import { definePreset } from '@primevue/themes'
import Aura from '@primevue/themes/aura'

import 'primeicons/primeicons.css'
import 'primeflex/primeflex.css'

const stylePreset = definePreset(Aura, {
  semantic: {
    primary: {
      50: '{green.50}',
      100: '{green.100}',
      200: '{green.200}',
      300: '{green.300}',
      400: '{green.400}',
      500: '{green.500}',
      600: '{green.600}',
      700: '{green.700}',
      800: '{green.800}',
      900: '{green.900}',
      950: '{green.950}',
    },
    colorScheme: {
      dark: {
        surface: {
          0: '#ffffff',
          50: '{neutral.50}',
          100: '{neutral.100}',
          200: '{neutral.200}',
          300: '{neutral.300}',
          400: '{neutral.400}',
          500: '{neutral.500}',
          600: '{neutral.600}',
          700: '{neutral.700}',
          800: '{neutral.800}',
          900: '{neutral.900}',
          950: '{neutral.950}',
        },
      },
    },
  },
})

import Button from 'primevue/button'
import Menu from 'primevue/menu'
import ScrollPanel from 'primevue/scrollpanel'
import DynamicDialog from 'primevue/dynamicdialog'
import DialogService from 'primevue/dialogservice'
import TabView from 'primevue/tabview'
import TabPanel from 'primevue/tabpanel'
import Textarea from 'primevue/textarea'
import Toolbar from 'primevue/toolbar'
import SelectButton from 'primevue/selectbutton'
import FileUpload from 'primevue/fileupload'
//import { AppImageUpdater } from 'electron-updater';

const routes = [
  {
    path: '/',
    name: 'Home',
    component: App,
  },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

const app = createApp(App)
app.use(PrimeVue, {
  theme: {
    preset: stylePreset,
    options: {
      darkModeSelector: 'system',
    },
  },
})
app.use(DialogService as any)
app.use(router as any)

app.component('Button', Button)
app.component('Toolbar', Toolbar)
app.component('Menu', Menu)
app.component('ScrollPanel', ScrollPanel)
app.component('DynamicDialog', DynamicDialog)
app.component('TabView', TabView)
app.component('TabPanel', TabPanel)
app.component('SelectButton', SelectButton)
app.component('Textarea', Textarea)
app.component('FileUpload', FileUpload)

app.mount('#app')
