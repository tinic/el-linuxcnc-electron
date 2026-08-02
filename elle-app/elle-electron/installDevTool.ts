import { installExtension, VUEJS_DEVTOOLS } from 'electron-devtools-installer'

export async function installExt() {
  try {
    await installExtension(VUEJS_DEVTOOLS)
  } catch {
    // devtools are best-effort in development; ignore failures
  }
}
