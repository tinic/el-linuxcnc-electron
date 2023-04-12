import { createApp } from 'vue'
import './style.css'
import App from './App.vue'
import PrimeVue from 'primevue/config';
import { createWebHistory, createRouter } from "vue-router";

import 'primevue/resources/primevue.min.css'            // core 
import 'primevue/resources/themes/md-dark-indigo/theme.css'  // theme
import 'primeicons/primeicons.css'                      // icons
import 'primeflex/primeflex.css'          // PrimeFlex

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
app.use(PrimeVue);
app.use(DialogService);
app.use(router)

app.component('Button', Button);
app.component('Toolbar', Toolbar);
app.component('Menu', Menu);
app.component('ScrollPanel', ScrollPanel);
app.component('DynamicDialog', DynamicDialog);
app.component('TabView', TabView);
app.component('TabPanel', TabPanel);
app.component('SelectButton', SelectButton);
app.component('Textarea', Textarea);

app.mount('#app')

