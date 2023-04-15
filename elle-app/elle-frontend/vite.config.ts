import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

// https://vitejs.dev/config/
export default defineConfig(({ mode }) => ({
  build: {
    outDir: './../dist',
    chunkSizeWarningLimit: 2000,
  },
  base: mode == 'development' ? '' : './',
  plugins: [vue({
    template: {
      compilerOptions: {
        isCustomElement: (tag) => ['font'].includes(tag),
      }
    }    
  })],
  server: {
      port: 3000,
      fs: {
        allow: ["../"]
    }
  },
}));
