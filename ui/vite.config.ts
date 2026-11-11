import vue from '@vitejs/plugin-vue'
import { defineConfig } from 'vitest/config'

// `base: './'` because the elle executable serves the build from its own URI scheme.
export default defineConfig({
  base: './',
  plugins: [vue()],
  server: { host: '127.0.0.1', port: 5173, strictPort: true },
  build: { outDir: 'dist', emptyOutDir: true, chunkSizeWarningLimit: 600 },
  test: { environment: 'jsdom', include: ['test/**/*.test.ts'] },
})
