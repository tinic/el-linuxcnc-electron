import { createApp } from 'vue'

import App from './App.vue'
import { start } from './machine'
import './style.css'

createApp(App).mount('#app')
start()
