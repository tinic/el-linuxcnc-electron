import { createApp } from 'vue'
import './style.css'
import App from './App.vue'
import PrimeVue from 'primevue/config';
import { createWebHistory, createRouter } from "vue-router";
import { definePreset } from '@primevue/themes';
import Aura from '@primevue/themes/aura';

import 'primeicons/primeicons.css'
import 'primeflex/primeflex.css'

const auraPreset = definePreset(Aura, {
    semantic: {
        primary: {
            50: '{slate.50}',
            100: '{slate.100}',
            200: '{slate.200}',
            300: '{slate.300}',
            400: '{slate.400}',
            500: '{slate.500}',
            600: '{slate.600}',
            700: '{slate.700}',
            800: '{slate.800}',
            900: '{slate.900}',
            950: '{slate.950}'
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
        preset: auraPreset,
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

