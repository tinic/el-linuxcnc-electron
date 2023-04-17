<script setup lang="ts">
import { ref, onMounted, computed, watch, defineAsyncComponent } from 'vue';
import { useDialog } from 'primevue/usedialog';
import { Box, Camera, LambertMaterial, MeshPublicInterface, PointLight, Renderer, RendererPublicInterface, Scene } from 'troisjs'
import * as THREE from 'three'
import { Line2, LineGeometry, LineMaterial, LineSegments2 } from 'three-fatline';

import Numpad from './components/Numpad.vue';
import DRODisplay from './components/DRODisplay.vue';

let halOutURL = 'http://localhost:8000/hal/hal_out';
let halInURL = 'http://localhost:8000/hal/hal_in';
let linuxcncURL = 'http://localhost:8001/linuxcnc/';

var userAgent = navigator.userAgent.toLowerCase();
if (userAgent.indexOf(' electron/') < 0) {
  halOutURL = 'http://lathev2:8000/hal/hal_out';
  halInURL = 'http://lathev2:8000/hal/hal_in';
  linuxcncURL = 'http://lathev2:8001/linuxcnc/';
}

const selectedMenu = ref(0);

// Polled over REST
const xpos = ref(0);
const zpos = ref(0);
const apos = ref(0);
const rpms = ref(0);

// Pushed over REST
const xpitch = ref(0.1);
const zpitch = ref(0.1);
const xpitchactive = ref(false);
const zpitchactive = ref(false);
const xstepperactive = ref(false);
const zstepperactive = ref(false);

// Internal
const numberentry = ref(0);
const xpitchlabel = ref('…');
const zpitchlabel = ref('…');
const xpitchangle = ref(0);
const metric = ref(true);

let zforward:boolean = true;
let xforward:boolean = true;
let xaxisoffset:number = 0;
let zaxisoffset:number = 0;
let aaxisoffset:number = 0;
let xaxisset:number = 0;
let zaxisset:number = 0;
let aaxisset:number = 0;
let xaxissetscheduled:boolean = false;
let zaxissetscheduled:boolean = false;
let aaxissetscheduled:boolean = false;

let buttonuptime:number = 0;
let buttondowntime:number = 0;
let buttonlefttime:number = 0;
let buttonrighttime:number = 0;
let buttonupscheduled:boolean = false;

enum FeedMode {
  none=0,
  longitudinal=1,
  cross=2,
  frontCompound=3,
  backCompound=4,
}
const selectedFeedMode = ref(FeedMode.longitudinal);

enum DirectionMode {
  none=0,
  forward=1,
  reverse=2,
  hold=3,
  idle=4
}
const selectedDirectionMode = ref(DirectionMode.forward);

const menuItems = ref([
    { separator: true },
    { label: 'Home', 
      icon: 'pi pi-fw pi-home',
      command: () => {
        selectedMenu.value = 0;
      }
    },
    { label: 'CC', 
      icon: 'pi pi-fw pi-link',
      command: () => {
        selectedMenu.value = 1;
      }
    },
    { label: 'HAL', 
      icon: 'pi pi-fw pi-link',
      command: () => {
        selectedMenu.value = 2;
      }
    },
    { label: 'Settings', 
      icon: 'pi pi-fw pi-cog',
      command: () => {
        selectedMenu.value = 3;
      }
    },
    { separator: true }
]); 

enum NumpadInputStage {
  none = 0,
  start = 1,
  entry = 2
};

const entryActive = ref(0);
let numpadInputStage = NumpadInputStage.none;
let numbersClicked = new Array<string>();
let numbersNegative = false;
let numbersPrevious:number = 0;

function treatOffClickAsEnter() {
  if (numpadInputStage == NumpadInputStage.start) {
    numberentry.value = numbersPrevious;
    setFinalNumber(numbersPrevious);
  } else if (numpadInputStage == NumpadInputStage.entry) {
    numberentry.value = calcNumber();
    setFinalNumber(numberentry.value);
  }
}

const numberClicked = (entry:number, value:number) => {
  treatOffClickAsEnter();
  numbersClicked.length = 0;
  numpadInputStage = NumpadInputStage.start;
  numberentry.value = numbersPrevious = metric.value ? value : (value / 25.4);
  entryActive.value = entry;
  numbersNegative = false;
};

function calcNumber():number {
  const dotIndex = numbersClicked.indexOf('.');
  let integerSize = 0;
  let fractionSize = 0;
  if (dotIndex >= 0) {
    integerSize = dotIndex;
    fractionSize = numbersClicked.length - dotIndex - 1;
  } else {
    integerSize = numbersClicked.length;
  }
  let value:number = 0;
  for (let i = 0; i < integerSize; i++) {
    value += (numbersClicked[i].charCodeAt(0)-0x30) * Math.pow(10,integerSize-i-1);
  }
  for (let i = 0; i < fractionSize; i++) {
    value += (numbersClicked[i+integerSize+1].charCodeAt(0)-0x30) * Math.pow(10,-i-1);
  }
  return value * (numbersNegative ? (-1) : (+1));;
}

function setFinalNumber(value:number) {
  if (!metric.value) {
    value = value * 25.4;
  }
  switch(entryActive.value) {
    case 1:
      xaxisset = value;
      xaxissetscheduled = true;
      xpos.value = value;
    break;
    case 2:
      zaxisset = value;
      zaxissetscheduled = true;
      zpos.value = value;
    break;
    case 3:
      aaxisset = value;
      aaxissetscheduled = true;
      apos.value = value;
    break;
    case 4:
      xpitch.value = Math.abs(value);
    break;
    case 5:
      zpitch.value = Math.abs(value);
    break;
  }
  numpadInputStage = NumpadInputStage.none;
  numbersClicked.length = 0;
  entryActive.value = 0;
}

const numPadClicked = (key:string) => {
  if (numpadInputStage == NumpadInputStage.none) {
    return;
  }
  switch(key) {
    case 'Escape':
      numpadInputStage = NumpadInputStage.none;
      numberentry.value = numbersPrevious;
      numbersClicked.length = 0;
      entryActive.value = 0;
    break;
    case 'Enter':
      if (numpadInputStage == NumpadInputStage.start) {
        numberentry.value = numbersPrevious;
        setFinalNumber(numbersPrevious);
      } else if (numpadInputStage == NumpadInputStage.entry) {
        numberentry.value = calcNumber();
        setFinalNumber(numberentry.value);
      }
    break;
    case 'Backspace':
      numpadInputStage = NumpadInputStage.entry;
      if (numbersClicked.at(-1) == '.') {
        numbersClicked.pop();
      }
      numbersClicked.pop();
      numberentry.value = calcNumber();
    break;
    case 'PlusMinus':
      if (numpadInputStage == NumpadInputStage.start) {
        numbersNegative = !numbersNegative;
        numberentry.value = numbersPrevious * (numbersNegative ? (-1) : (+1));
        setFinalNumber(numberentry.value);
      } else {
        numbersNegative = !numbersNegative;
        numberentry.value = calcNumber();
      }
    break;
    case 'Third':
      if (numpadInputStage == NumpadInputStage.start) {
        numberentry.value = numberentry.value / 3;
        setFinalNumber(numberentry.value);
      }
    break;
    case 'Half':
      if (numpadInputStage == NumpadInputStage.start) {
        numberentry.value = numberentry.value / 2;
        setFinalNumber(numberentry.value);
      }
    break;
    default:
      numpadInputStage = NumpadInputStage.entry;
      numbersClicked.push(key);
      numberentry.value = calcNumber();
    break;
  }
}

const zeroClicked = (entry:number) => {
  treatOffClickAsEnter();
  entryActive.value = 0;
  switch(entry) {
    case 1:
      xaxisset = 0;
      xaxissetscheduled = true;
      scheduleHALOut();
    break;
    case 2:
      zaxisset = 0;
      zaxissetscheduled = true;
      scheduleHALOut();
    break;
    case 3:
      aaxisset = 0;
      aaxissetscheduled = true;
      scheduleHALOut();
    break;
  }
};

const metricClicked = () => {
  treatOffClickAsEnter();
  metric.value = !metric.value;
};

const otherClicked = () => {
  treatOffClickAsEnter();
  entryActive.value = 0;
};

const halStdoutText = ref('');

const startHAL = () => {
  halStdoutText.value = '';
  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(' electron/') > -1) {
    window.api.send('startHAL');
  }
}

const stopHAL = () => {
  halStdoutText.value = '';
  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(' electron/') > -1) {
    window.api.send('stopHAL');
  }
}

const quitApplication = () => {
  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(' electron/') > -1) {
    window.api.send('quit');
  }
};

const rendererC = ref()

class Backplot {

  json:any;

  xmin:number;
  ymin:number;
  zmin:number;
  xmax:number;
  ymax:number;
  zmax:number;

  fmin:number=+999999;
  fmax:number=-999999;

  lmin:number=+999999;
  lmax:number=-999999;

  tlin:number=0;

  xoff:number;
  yoff:number;
  zoff:number;
  scal:number;
  omat:THREE.Matrix4 = new THREE.Matrix4();

  boundingBoxMaterial:LineMaterial = new LineMaterial( { 
		color: 0xaaaaaa, 
		linewidth: 1,
    resolution: new THREE.Vector2(800,600)
  });

  backplotMaterial:LineMaterial = new LineMaterial( { 
		color: 0xffffff, 
		linewidth: 1,
    resolution: new THREE.Vector2(800,600)
  });

  backplotMaterial0:Array<LineMaterial> = new Array<LineMaterial>();
  backplotMaterial1:Array<LineMaterial> = new Array<LineMaterial>();
  backplotMaterial2:LineMaterial;
  backplotMaterial3:LineMaterial;
    
  constructor(_json:any) {
    this.json = _json;

    let color0 = new THREE.Color(0xff0000);
    let color1 = new THREE.Color(0x0000ff);

    this.backplotMaterial2 = new LineMaterial( { 
      color: 0xff00ff, 
      linewidth: 1,
      resolution: new THREE.Vector2(800,600)
    })

    this.backplotMaterial3 = new LineMaterial( { 
      color: 0xff00ff, 
      linewidth: 3,
      resolution: new THREE.Vector2(800,600)
    })

    for (let c = 0; c < 256; c++) {
      this.backplotMaterial0[c] = new LineMaterial( { 
        color: color0.lerpHSL(color1, c/32768).getHex(), 
        linewidth: 1,
        resolution: new THREE.Vector2(800,600)
      })

      this.backplotMaterial1[c] = new LineMaterial( { 
        color: color0.lerpHSL(color1, c/32768).getHex(), 
        linewidth: 4,
        resolution: new THREE.Vector2(800,600)
      })
    }

    this.xmin = this.json["extents"][0];
    this.ymin = this.json["extents"][1];
    this.zmin = this.json["extents"][2];
    this.xmax = this.json["extents"][3];
    this.ymax = this.json["extents"][4];
    this.zmax = this.json["extents"][5];

    this.xoff = -(this.xmax - this.xmin) / 2 - this.xmin;
    this.yoff = -(this.ymax - this.ymin) / 2 - this.ymin;
    this.zoff = -(this.zmax - this.zmin) / 2 - this.zmin;

    let maxx = Math.abs(this.xmax - this.xmin);
    let maxy = Math.abs(this.ymax - this.ymin);
    let maxz = Math.abs(this.zmax - this.zmin);

    this.scal  = 1.0 / Math.max(maxz,Math.max(maxx, maxy))

    let smat = new THREE.Matrix4().makeScale(this.scal, this.scal, this.scal);
    let tmat = new THREE.Matrix4().makeTranslation(this.xoff, this.yoff, this.zoff)

    this.omat.identity()
    this.omat.multiply(smat)
    this.omat.multiply(tmat)

    this.xmin += this.xoff - 0.01;
    this.xmax += this.xoff + 0.01;
    this.ymin += this.yoff - 0.01;
    this.ymax += this.yoff + 0.01;
    this.zmin += this.zoff - 0.01;
    this.zmax += this.zoff + 0.01;

    this.xmin *= this.scal;
    this.xmax *= this.scal;
    this.ymin *= this.scal;
    this.ymax *= this.scal;
    this.zmin *= this.scal;
    this.zmax *= this.scal;

    this.createLine2Geometry()
  }

  createLine2Geometry() {
    for (let entry of this.json["backplot"]) {
      let entryType = entry["type"]
      if (entry.hasOwnProperty("line")) {
        this.lmin = Math.min(this.lmin, entry["line"]);
        this.lmax = Math.max(this.lmax, entry["line"]);
      }
      switch(entryType) {
        case 'feed':
        case 'trav':
        for (let line of entry[entryType]) {
          if (entryType == "feed") {
            if (line.hasOwnProperty("rate")) {
              this.fmin = Math.min(this.fmin, line["rate"]);
              this.fmax = Math.max(this.fmax, line["rate"]);
            }
          }
          let points:Array<number> = [];
          let l0 = new THREE.Vector3( 
            line["coords"][0], 
            line["coords"][1], 
            line["coords"][2]).applyMatrix4(this.omat);
          let l1 = new THREE.Vector3( 
            line["coords"][3], 
            line["coords"][4], 
            line["coords"][5]).applyMatrix4(this.omat);
          points.push(l0.x, l0.y, l0.z, l1.x, l1.y, l1.z);
          line['points'] = points;
          var geometry:LineGeometry = new LineGeometry();
          geometry.setPositions(points);
          line['geometry'] = geometry;
          let line2 = new Line2(geometry, this.backplotMaterial);
          line2.computeLineDistances();
          line['line2'] = line2;
          this.tlin++;
        }
        break;
        case 'arcfeed':
        // TODO!!!!!
        break;
        case 'dwell':
        // TODO!!!!!
        break;
      }
    }
  }

  addBackplotToScene() {
    const renderer = rendererC.value as RendererPublicInterface
    for (let entry of this.json["backplot"]) {
      let entryType = entry["type"]
      switch(entryType) {
        case 'feed':
        case 'trav':
        entry['vector3'] = []
        for (let line of entry[entryType]) {
          if (line.hasOwnProperty("line2")) {
            let line2 = line["line2"] as Line2;
            line2.computeLineDistances();
            renderer.scene?.add( line2 );
          }
          break;
        }
        break;
        case 'arcfeed':
        break;
        case 'dwell':
        break;
      }
    }
  }

  addBoundingBoxToScene() {
    const renderer = rendererC.value as RendererPublicInterface
    let geom_lines = this.boundingBoxLine2Points();
    for (let geom_line of geom_lines) {
      var geometry = new LineGeometry();
      geometry.setPositions(geom_line);
      let line = new Line2( geometry,  this.boundingBoxMaterial);
      line.computeLineDistances();
      renderer.scene?.add( line );
    }
  }

  updateProgress(lineidx:number) {
    let clin:number = 0;
    for (let entry of this.json["backplot"]) {
      let entryType = entry["type"]
      switch(entryType) {
        case 'feed':
        case 'trav':
        for (let line of entry[entryType]) {
          let line2 = line['line2'] as Line2;
          if (entryType == "feed" && line.hasOwnProperty("rate")) {
            let r = Math.floor(((clin / this.tlin) * 256) % 256);
            line2.material = clin > lineidx ? this.backplotMaterial0[r] : this.backplotMaterial1[r];
          } else {
            line2.material = clin > lineidx ? this.backplotMaterial2 : this.backplotMaterial3;
          }
          clin++;
        }
        break;
        case 'arcfeed':
        break;
        case 'dwell':
        break;
      }
    }
  }

  boundingBoxLine2Points():Array<Array<number>> {
    let a:Array<Array<number>> = [];
    a.push([this.xmin,this.ymin,this.zmin,this.xmax,this.ymin,this.zmin])
    a.push([this.xmax,this.ymin,this.zmin,this.xmax,this.ymax,this.zmin])
    a.push([this.xmax,this.ymax,this.zmin,this.xmin,this.ymax,this.zmin])
    a.push([this.xmin,this.ymax,this.zmin,this.xmin,this.ymin,this.zmin])
    a.push([this.xmin,this.ymin,this.zmax,this.xmax,this.ymin,this.zmax])
    a.push([this.xmax,this.ymin,this.zmax,this.xmax,this.ymax,this.zmax])
    a.push([this.xmax,this.ymax,this.zmax,this.xmin,this.ymax,this.zmax])
    a.push([this.xmin,this.ymax,this.zmax,this.xmin,this.ymin,this.zmax])
    a.push([this.xmin,this.ymin,this.zmin,this.xmin,this.ymin,this.zmax])
    a.push([this.xmax,this.ymin,this.zmin,this.xmax,this.ymin,this.zmax])
    a.push([this.xmax,this.ymax,this.zmin,this.xmax,this.ymax,this.zmax])
    a.push([this.xmin,this.ymax,this.zmin,this.xmin,this.ymax,this.zmax])
    return a;
  }

}

const gcodeUploader = async (event:any) => {
    const file = event.files[0];
    const reader = new FileReader();
    reader.readAsText(file);
    reader.onloadend = function () {
        putLinuxCNC('backplot', {"gcode": btoa(reader.result as string)}).then(json => {

          const renderer = rendererC.value as RendererPublicInterface
          renderer.scene?.clear();

          let backplot = new Backplot(json);
          backplot.addBoundingBoxToScene()
          backplot.addBackplotToScene()

          let xlin:number = 0;
          let xinc:number = backplot.tlin / 600;
          renderer.onBeforeRender(() => {
            backplot.updateProgress(xlin);
            xlin += xinc;
            xlin %= backplot.tlin;
          })
        })
      }
};

let halOutScheduled:boolean = false;
let updateInterval:NodeJS.Timer;

interface HalIn {
  position_z:number,
  position_x:number,
  position_a:number,
  speed_rps:number
};

async function putHalOut(halOut:Object) {
  try {
    const response = await fetch(halOutURL, {
      method: "PUT",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(halOut),
    });
    const result = await response.json();
    return result;
  } catch {
    // nop
  }
  return {};
}

async function putLinuxCNC(command:string, data:Object) {
  try {
    const response = await fetch(linuxcncURL + command, {
      method: "PUT",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(data),
    });
    const result = await response.json();
    return result;
  } catch {
    // nop
  }
  return {};
}

function getHalIn(): Promise<HalIn[]> {
  return fetch(halInURL)
                .then(res => res.json())
                .then(res => {
                  return res as HalIn[]
                })
};

function startPoll() {
  updateInterval = setInterval(() => {
    try {
      getHalIn().then(halIn => {
        if (xaxissetscheduled) {
          xaxissetscheduled = false;
          xaxisoffset = (-(halIn as any).position_x) - xaxisset;
          xaxisset = 0;
        }
        if (zaxissetscheduled) {
          zaxissetscheduled = false;
          zaxisoffset = (halIn as any).position_z - zaxisset;
          zaxisset = 0;
        }
        if (aaxissetscheduled) {
          aaxissetscheduled = false;
          aaxisoffset = (halIn as any).position_a - ((aaxisset / 360) % 1);
          aaxisset = 0;
        }
        zpos.value = (halIn as any).position_z - zaxisoffset;
        xpos.value = (-(halIn as any).position_x) - xaxisoffset;
        apos.value = Math.abs((((halIn as any).position_a - aaxisoffset) % 1) * 360);
        rpms.value = Math.abs((halIn as any).speed_rps * 60);
      });
    } catch {
      // nop
    }
    if (buttonuptime > 0) {
      let velocity = (Date.now()/1000 - buttonuptime) * 3;
			velocity = Math.min(velocity, 3.0)
      let halOut = {
          "control_z_type" : 1,
          "velocity_z_cmd" : +velocity
      }
      putHalOut(halOut);
    }
    if (buttondowntime > 0) {
      let velocity = (Date.now()/1000 - buttondowntime) * 3;
			velocity = Math.min(velocity, 3.0)
      let halOut = {
          "control_z_type" : 1,
          "velocity_z_cmd" : -velocity
      }
      putHalOut(halOut);
    }
    if (buttonlefttime > 0) {
      let velocity = (Date.now()/1000 - buttonlefttime) * 3;
			velocity = Math.min(velocity, 6.0)
      let halOut = {
          "control_x_type" : 1,
          "velocity_x_cmd" : -velocity
      }
      putHalOut(halOut);
    }
    if (buttonrighttime > 0) {
      let velocity = (Date.now()/1000 - buttonrighttime) * 3;
			velocity = Math.min(velocity, 6.0)
      let halOut = {
          "control_x_type" : 1,
          "velocity_x_cmd" : +velocity
      }
      putHalOut(halOut);
    }
    if (buttonupscheduled) {
      buttonupscheduled = false
      buttonuptime = 0;
      buttondowntime = 0;
      buttonlefttime = 0;
      buttonrighttime = 0;
      let halOut = {
          "velocity_x_cmd" : 0,
          "control_x_type" : 0,
          "velocity_y_cmd" : 0,
          "control_y_type" : 0,
      }
      putHalOut(halOut);
    }
    if (halOutScheduled) {
      halOutScheduled = false;
      let halOut = {
          "control_source" : false,
          "forward_z" : zforward ? -zpitch.value : zpitch.value,
          "forward_x" : xforward ? -xpitch.value : xpitch.value,
          "enable_z" : zpitchactive.value,
          "enable_x" : xpitchactive.value,
          "enable_stepper_z" : zstepperactive.value,
          "enable_stepper_x" : xstepperactive.value
      };
      putHalOut(halOut);
    }
  }, 33.33333);
}

function endPoll() {
  clearTimeout(updateInterval);
}

const forwardIcon = computed(() => {
  switch (selectedFeedMode.value) {
    case FeedMode.longitudinal:
    return '⬅'
    case FeedMode.cross:
    return '⬆'
    case FeedMode.frontCompound:
    return '⬋'
    case FeedMode.backCompound:
    return '⬉'
  }
});

const reverseIcon = computed(() => {
  switch (selectedFeedMode.value) {
    case FeedMode.longitudinal:
    return '⮕'
    case FeedMode.cross:
    return '⬇'
    case FeedMode.frontCompound:
    return '⬈'
    case FeedMode.backCompound:
    return '⬊'
  }
});

const feedModeLongitudinalClicked = () => {
  selectedFeedMode.value = FeedMode.longitudinal;
}
const feedModeCrossClicked = () => {
  selectedFeedMode.value = FeedMode.cross;
}
const feedModeFrontCompoundClicked = () => {
  selectedFeedMode.value = FeedMode.frontCompound;
}
const feedModeBackCompoundClicked = () => {
  selectedFeedMode.value = FeedMode.backCompound;
}
const directionModeForwardClicked = () => {
  selectedDirectionMode.value = DirectionMode.forward;
}
const directionModeReverseClicked = () => {
  selectedDirectionMode.value = DirectionMode.reverse;
}
const directionModeHoldClicked = () => {
  selectedDirectionMode.value = DirectionMode.hold;
}
const directionModeIdleClicked = () => {
  selectedDirectionMode.value = DirectionMode.idle;
}

const mouseUpUp = () => {
  buttonupscheduled = true;
}

const mouseDownUp = () => {
  buttonuptime = Date.now() / 1000;
}

const mouseUpLeft = () => {
  buttonupscheduled = true;
}

const mouseDownLeft = () => {
  buttonlefttime = Date.now() / 1000;
}

const mouseUpRight = () => {
  buttonupscheduled = true;
}

const mouseDownRight = () => {
  buttonrighttime = Date.now() / 1000;
}

const mouseUpDown = () => {
  buttonupscheduled = true;
}

const mouseDownDown = () => {
  buttondowntime = Date.now() / 1000;
}

function scheduleHALOut() {
  halOutScheduled = true;
}

function updateHALOut() {
  switch(selectedFeedMode.value) {
    case FeedMode.longitudinal:
    switch(selectedDirectionMode.value) {
      case DirectionMode.forward:
        zstepperactive.value = true;
        xstepperactive.value = false;
        zpitchactive.value = true;
        xpitchactive.value = false;
        zforward = true;
        xforward = true;
      break;
      case DirectionMode.reverse:
        zstepperactive.value = true;
        xstepperactive.value = false;
        zpitchactive.value = true;
        xpitchactive.value = false;
        zforward = false;
        xforward = false;
      break;
      case DirectionMode.hold:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = true;
        xpitchactive.value = false;
        zforward = true;
        xforward = true;
      break;
      case DirectionMode.idle:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = false;
        xpitchactive.value = false;
        zforward = true;
        xforward = true;
      break;
    }
    break;
    case FeedMode.cross:
    switch(selectedDirectionMode.value) {
      case DirectionMode.forward:
        zstepperactive.value = false;
        xstepperactive.value = true;
        zpitchactive.value = false;
        xpitchactive.value = true;
        zforward = true;
        xforward = false;
      break;
      case DirectionMode.reverse:
        zstepperactive.value = false;
        xstepperactive.value = true;
        zpitchactive.value = false;
        xpitchactive.value = true;
        zforward = false;
        xforward = true;
      break;
      case DirectionMode.hold:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = false;
        xpitchactive.value = true;
        zforward = true;
        xforward = true;
      break;
      case DirectionMode.idle:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = false;
        xpitchactive.value = false;
        zforward = true;
        xforward = true;
      break;
    }
    break;
    case FeedMode.frontCompound:
    switch(selectedDirectionMode.value) {
      case DirectionMode.forward:
        zstepperactive.value = true;
        xstepperactive.value = true;
        zpitchactive.value = true;
        xpitchactive.value = true;
        zforward = true;
        xforward = true;
      break;
      case DirectionMode.reverse:
        zstepperactive.value = true;
        xstepperactive.value = true;
        zpitchactive.value = true;
        xpitchactive.value = true;
        zforward = false;
        xforward = false;
      break;
      case DirectionMode.hold:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = true;
        xpitchactive.value = true;
        zforward = true;
        xforward = true;
      break;
      case DirectionMode.idle:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = false;
        xpitchactive.value = false;
        zforward = true;
        xforward = true;
      break;
    }
    break;
    case FeedMode.backCompound:
    switch(selectedDirectionMode.value) {
      case DirectionMode.forward:
        zstepperactive.value = true;
        xstepperactive.value = true;
        zpitchactive.value = true;
        xpitchactive.value = true;
        zforward = true;
        xforward = false;
      break;
      case DirectionMode.reverse:
        zstepperactive.value = true;
        xstepperactive.value = true;
        zpitchactive.value = true;
        xpitchactive.value = true;
        zforward = false;
        xforward = true;
      break;
      case DirectionMode.hold:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = true;
        xpitchactive.value = true;
        zforward = true;
        xforward = false;
      break;
      case DirectionMode.idle:
        zstepperactive.value = false;
        xstepperactive.value = false;
        zpitchactive.value = false;
        xpitchactive.value = false;
        zforward = true;
        xforward = false;
      break;
    }
    break;
  }
  scheduleHALOut();
}

watch([selectedFeedMode, selectedDirectionMode], () => {
  updateHALOut();
});

watch([zpitch, xpitch], () => {
  updateHALOut();
})

const PitchSelector = defineAsyncComponent(() => import('./components/PitchSelector.vue'));

function pitchForAngle(pitch:number, angle:number) {
  return pitch * Math.tan(angle * (Math.PI / 180))
}

const dialog = useDialog();
const pitchClicked = (axis:string) => {
  treatOffClickAsEnter();
  entryActive.value = 0;
  const dialogRef = dialog.open(PitchSelector, {
        props: {
            header: 'Select Pitch',
            style: {
                width: '70vw',
            },
            breakpoints:{
                '960px': '75vw',
                '640px': '90vw'
            },
            position: 'top',
            modal: true,
        },
        data: {
          axis: axis
        },
        emits: {
          onSelected: (axis:string, name:string, value:number, type:string) => {
            switch(axis) {
              case 'z':
                if (type != 'angle') {
                  zpitch.value = value;
                  zpitchlabel.value = name;
                  if (xpitchangle.value > 0) {
                    xpitch.value = pitchForAngle(zpitch.value, xpitchangle.value);
                  }
                }
                break;
              case 'x':
                if (type != 'angle') {
                  xpitch.value = value;
                  xpitchlabel.value = name;
                  xpitchangle.value = 0;
                } else {
                  xpitch.value = pitchForAngle(zpitch.value, value);
                  xpitchlabel.value = name;
                  xpitchangle.value = value;
                }
                break;
            }
          }
        },
        templates: {
        },
        onClose: (options) => {
        }
    });
};

onMounted(() => {

  var userAgent = navigator.userAgent.toLowerCase();
  if (userAgent.indexOf(' electron/') > -1) {
    window.api.receive('halStarted', () => {
      selectedMenu.value = 0;
      startPoll();
      updateHALOut();
    });

    window.api.receive('halStopped', () => {
      endPoll();
    });

    window.api.receive('halStdout', (event:any, arg:any) => {
      halStdoutText.value += event as string;
    });

    selectedMenu.value = 2;
    startHAL();
  } else {
    startPoll();
    updateHALOut();
  }

})

</script>

<script lang="ts">

</script>

<template>
  <div class="flex flex-row flex-grow-1 absolute top-0 left-0 wrapper bg-gray-800">
    <Menu v-model="selectedMenu" :model="menuItems" class="flex-none">
      <template #start>
        <button class="w-full p-link flex align-items-center p-2 pl-3 text-color hover:surface-200 border-noround">
          <div class="flex flex-column align">
          <span class="font-bold">Elle</span>
          </div>
        </button>
      </template>
      <template #end>
        <button @click="quitApplication" class="w-full p-link bottom-0flex align-items-center p-2 pl-4 text-color hover:surface-200 border-noround ">
          <i class="pi pi-sign-out" />
          <span class="ml-2">Exit</span>
        </button>
      </template>
    </Menu>
    <div v-if="selectedMenu==0" class="m-2">
      <div class="flex flex-row">
        <DRODisplay class="mr-2 h-min"
          :entryActive="entryActive"
          :xpos="xpos"
          :zpos="zpos"
          :apos="apos"
          :rpms="rpms"
          :xpitch="xpitch"
          :zpitch="zpitch"
          :xlock="xpitchactive"
          :zlock="zpitchactive"
          :xpitchactive="xstepperactive"
          :zpitchactive="zstepperactive"
          :numberentry="numberentry"
          :xpitchlabel="xpitchlabel"
          :zpitchlabel="zpitchlabel"
          :metric="metric"
          @numberClicked="numberClicked"
          @zeroClicked="zeroClicked"
          @pitchClicked="pitchClicked"
          @metricClicked="metricClicked"
          @otherClicked="otherClicked"/>
          <Numpad class=""
          @numPadClicked="numPadClicked"/>
        </div>
        <div class="flex flex-row">
          <div class="grid dro-font-mode grid-nogutter p-3 pr-4 m-0 mt-2 bg-gray-900" style="width:16em">
            <div class="col-12 align-content-center">Feed</div>
            <button @click="feedModeLongitudinalClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedFeedMode == FeedMode.longitudinal" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                ⬌ Longitudinal
              </span>
            </button>
            <button @click="feedModeCrossClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedFeedMode == FeedMode.cross" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                ⬍ Cross
              </span>
            </button>
            <button @click="feedModeFrontCompoundClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedFeedMode == FeedMode.frontCompound" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                ⬋ Front Compound
              </span>
            </button>
            <button @click="feedModeBackCompoundClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedFeedMode == FeedMode.backCompound" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                ⬉ Back Compound
              </span>
            </button>
          </div>
          <div class="grid dro-font-mode grid-nogutter p-3 pr-4 m-0 mt-2 bg-gray-900" style="width:15em">
            <div class="col-12 align-content-center">Direction</div>
            <button @click="directionModeForwardClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedDirectionMode == DirectionMode.forward" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                {{ forwardIcon }} Forward
              </span>
            </button>
            <button @click="directionModeReverseClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedDirectionMode == DirectionMode.reverse" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                {{ reverseIcon }} Reverse
              </span>
            </button>
            <button @click="directionModeHoldClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedDirectionMode == DirectionMode.hold" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                ⏸ Hold
              </span>
            </button>
            <button @click="directionModeIdleClicked" size="large" class="col-12 dro-font-mode button-mode p-3 m-1">
              <span class="flex flex-row align-items-center">
                <i v-if="selectedDirectionMode == DirectionMode.idle" class="pi pi-circle-fill mr-3" style="color:#ff0000"/>
                <i v-else class="pi pi-circle mr-3"/>
                ⏹ Idle
              </span>
            </button>
          </div>
          <div class="grid grid-nogutter bg-gray-900 mt-2 p-1 dro-font-mode" style="width:24em">
              <div class="col-4 p-1"></div>
              <div class="col-4 p-1"><button @mouseup="mouseUpUp" @mousedown="mouseDownUp" class="button-mode button-direction w-full h-full">⏶</button></div>
              <div class="col-4 p-1"></div>
              <div class="col-4 p-1"><button @mouseup="mouseUpLeft" @mousedown="mouseDownLeft" class="button-mode button-direction w-full h-full">⏴</button></div>
              <div class="col-4 p-1"><button class="button-mode button-direction w-full h-full">STOP</button></div>
              <div class="col-4 p-1"><button @mouseup="mouseUpRight" @mousedown="mouseDownRight" class="button-mode button-direction w-full h-full">⏵</button></div>
              <div class="col-4 p-1"></div>
              <div class="col-4 p-1"><button @mouseup="mouseUpDown" @mousedown="mouseDownDown" class="button-mode button-direction w-full h-full">⏷</button></div>
              <div class="col-4 p-1"></div>
            </div>
        </div>
        <DynamicDialog/>
    </div>
    <div v-if="selectedMenu==1" class="flex-grow-1 flex align-items-center justify-content-center bg-blue-500 ">
      <div class="flex flex-column">
        <FileUpload mode="basic" name="elle[]" url="/api/upload" accept="text/plain" customUpload :auto="true" @uploader="gcodeUploader" />
        <Renderer ref="rendererC" antialias :orbit-ctrl="{ enableDamping: true }" width="800" height="600">
          <Camera :position="{ z: 1.5 }" />
          <Scene>
          </Scene>
        </Renderer>
      </div>
    </div>
    <div v-if="selectedMenu==2" class="flex-grow-1 flex align-items-center m-2 justify-content-center ">
      <div class="flex flex-column w-full h-full">
        <Toolbar class="mb-2 bg-gray-900">
          <template #start>
              <Button @click="startHAL" label="Start HAL" icon="pi pi-play" class="mr-2" />
              <Button @click="stopHAL" label="Stop HAL" icon="pi pi-stop" severity="success" />
          </template>
        </Toolbar>
        <Textarea v-model="halStdoutText" autoScroll="true" rows="30" cols="30" />
      </div>
    </div>
    <div v-if="selectedMenu==3" class="flex-grow-1 flex align-items-center justify-content-center bg-blue-500 ">
      Settings
    </div>
  </div>
</template>

<style scoped>
.button-mode {
    background: #333;
    color: #ffffff;
}

.button-direction {
  font-size: 1.5em;
}

.dro-font-mode {
  font-family: 'iosevka';
  font-weight: bold;
  font-size: 1.1em;
  text-align: center;
}

.fixed-width-font {
  font-family: 'iosevka';
  font-weight: normal;
}

.wrapper, html, body {
    width: 100%;
    height: 100%;
    margin: 0;
    color: #ffffff;
    background-color: #222222;
}

.wrapper {
    display: flex;
    flex-direction: column;
}
</style>
