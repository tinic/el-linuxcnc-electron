import { createApp } from 'vue'
import './style.css'
import App from './App.vue'
import PrimeVue from 'primevue/config';

import 'primevue/resources/primevue.min.css'            // core 
import 'primevue/resources/themes/md-dark-indigo/theme.css'  // theme
import 'primeicons/primeicons.css'                      // icons
import 'primeflex/primeflex.css'          // PrimeFlex

import Button from "primevue/button"
import Toolbar from "primevue/toolbar"
import Menu from "primevue/menu"
import ScrollPanel from 'primevue/scrollpanel';

const app = createApp(App);
app.use(PrimeVue);
app.component('Button', Button);
app.component('Toolbar', Toolbar);
app.component('Menu', Menu);
app.component('ScrollPanel', ScrollPanel);
app.mount('#app')
