import { createApp } from 'vue'
import './style.css'
import App from './App.vue'
import PrimeVue from 'primevue/config';
import { createWebHistory, createRouter } from "vue-router";
import { definePreset } from '@primevue/themes';
import Aura from '@primevue/themes/aura';

import 'primeicons/primeicons.css'                      // icons
import 'primeflex/primeflex.css'          // PrimeFlex

const MyPreset = definePreset(Aura, {
    semantic: {
        primary: {
            50: '{indigo.50}',
            100: '{indigo.100}',
            200: '{indigo.200}',
            300: '{indigo.300}',
            400: '{indigo.400}',
            500: '{indigo.500}',
            600: '{indigo.600}',
            700: '{indigo.700}',
            800: '{indigo.800}',
            900: '{indigo.900}',
            950: '{indigo.950}'
        }
    }
});

import Button from "primevue/button";
import Menu from "primevue/menu";
import ScrollPanel from 'primevue/scrollpanel';
import DynamicDialog from 'primevue/dynamicdialog';
import DialogService from 'primevue/dialogservice';
import TabView from 'primevue/tabview';
import TabPanel from 'primevue/tabpanel';
import Textarea from 'primevue/textarea';
import Toolbar from "primevue/toolbar";
import SelectButton from 'primevue/selectbutton';
import FileUpload from 'primevue/fileupload';
//import { AppImageUpdater } from 'electron-updater';

const routes = [
    {
      path: "/",
      name: "Home",
      component: App,
    }
];
  
const router = createRouter({
    history: createWebHistory(),
    routes
});

const app = createApp(App);
app.use(PrimeVue, {
    theme: {
        preset: MyPreset,
        options: {
            darkModeSelector: 'system'
        }
    }
});
app.use(DialogService as any);
app.use(router as any);

app.component('Button', Button);
app.component('Toolbar', Toolbar);
app.component('Menu', Menu);
app.component('ScrollPanel', ScrollPanel);
app.component('DynamicDialog', DynamicDialog);
app.component('TabView', TabView);
app.component('TabPanel', TabPanel);
app.component('SelectButton', SelectButton);
app.component('Textarea', Textarea);
app.component('FileUpload', FileUpload);

app.mount('#app')

