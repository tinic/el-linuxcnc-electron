# Elle

## Description
Elle is a standalone electronic leadscrew, linuxcnc HAL configuration, Vue3 based Electron application for CNC converted lathes. It brings back manual control to lathes which have been converted to CNC. In addition in can run a few common select canned cycles which cover 99% of what is done with manual lathes.

The UI is primarily designed for touch screens, keyboard is not required. Where numbers can be entered a simple on screen number pad can be used. The interface mostly behaves like a classic DRO.

The application operates in two modes: Manual and Canned Cycle which can be switched at will. Both share a single coordinate system which can be set up in manual mode and is picked up by the canned cycles. 

More features:

- Diameter and Radius mode are suported. 
- mm and inch can be switched on the fly. All numbers in the interface are updated dynamically.
- The threading and turning cycles include many common presets including NPT which are traditionally not easy to do on manual lathes.
- A simple jog interface is included in the manual mode.
- Axis can be set into different modes like hold (steppers are locked), idle (steppers are unpowered) and engaged (steppers are in sync with the spindle).
- The display of spindle rotation is in degrees and can be used like a rotary table.
- Before a canned cycle is run, the user is presented with an animated backplot preview of the g-code which is about to run.

## File Structure
- `elle-frontend/src/`: Contains the Vue3 application code.
    - `components/`: Vue components.
    - `composable/`: Vue composabes.
    - `assets/`: Static assets like fonts and presets for canned cycles.
    - `App.vue`: Main entry point of the application.
- `elle-electron/`: Contains the Electron application code.
    - `main.ts`: Main entry point of the Electron application. Mostly used to enable dev console.
- `elle-hal/`: Contains linuxcnc hal and ini configuration and glue python code.
    - `lathe_halcomp.py`: main rest interface used by the Vue application. Also contains code to generate canned cycles and back plots.

## Dependencies
- `vue`: Core Vue library.
- `threejs`: Used for rendering back plots.
- `primevue4`: Used for interface elements, css and icons
- `vite`: For build
- `electron`: Application packager

## Coding Style
- Follow common Typescript style guide.
- Follow the Vue style guide.
- Follow the Primevue style guide.
- Follow best python practices.
- Clean code, Duplication of code is avoided.
- Components share patterns and anticipate extensions.
- Using composables where possible.
- Split UI into components where possible.

## Commands
- `yarn start`: Start the development server and the electron app. Will also start linuxcnc + hal backend once the app is up and running.
- `yarn build`: Build the application.
- `yarn app:build`: Build the application for production.
