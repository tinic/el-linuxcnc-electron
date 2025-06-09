let halOutURL = "http://localhost:8000/hal/hal_out";
let halInURL = "http://localhost:8000/hal/hal_in";
let linuxcncURL = "http://localhost:8001/linuxcnc/";
let threadingURL = "http://localhost:8000/hal/threading";
let threadingGenerateURL = "http://localhost:8000/hal/threading/generate";
let turningURL = "http://localhost:8000/hal/turning";
let turningGenerateURL = "http://localhost:8000/hal/turning/generate";
let cleanupURL = "http://localhost:8000/hal/cleanup";
let abortURL = "http://localhost:8000/hal/abort";
let estopURL = "http://localhost:8000/hal/estop";

var userAgent = navigator.userAgent.toLowerCase();
if (userAgent.indexOf(" electron/") < 0) {
  halOutURL = "http://lathev2:8000/hal/hal_out";
  halInURL = "http://lathev2:8000/hal/hal_in";
  linuxcncURL = "http://lathev2:8001/linuxcnc/";
  threadingURL = "http://lathev2:8000/hal/threading";
  threadingGenerateURL = "http://lathev2:8000/hal/threading/generate";
  turningURL = "http://lathev2:8000/hal/turning";
  turningGenerateURL = "http://lathev2:8000/hal/turning/generate";
  cleanupURL = "http://lathev2:8000/hal/cleanup";
  abortURL = "http://lathev2:8000/hal/abort";
  estopURL = "http://lathev2:8000/hal/estop";
}

export interface HalIn {
  position_z: number;
  position_x: number;
  position_a: number;
  speed_rps: number;
}


export async function putThreading(threadingParams: Object) {
  try {
    const response = await fetch(threadingURL, {
      method: "PUT",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(threadingParams),
    });
    const result = await response.json();
    return result;
  } catch {
    // nop
  }
  return {};
}

export async function generateThreadingGcode(threadingParams: Object) {
  try {
    const response = await fetch(threadingGenerateURL, {
      method: "PUT",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(threadingParams),
    });
    const result = await response.json();
    return result;
  } catch {
    // nop
  }
  return {};
}

export async function putTurning(turningParams: Object) {
  try {
    const response = await fetch(turningURL, {
      method: "PUT",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(turningParams),
    });
    const result = await response.json();
    return result;
  } catch {
    // nop
  }
  return {};
}

export async function generateTurningGcode(turningParams: Object) {
  try {
    const response = await fetch(turningGenerateURL, {
      method: "PUT",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(turningParams),
    });
    const result = await response.json();
    return result;
  } catch {
    // nop
  }
  return {};
}

export async function cleanupCannedCycles() {
  try {
    const response = await fetch(cleanupURL, {
      method: "PUT",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({}),
    });
    const result = await response.json();
    return result;
  } catch {
  }
  return {};
}

export async function putAbort() {
  try {
    const response = await fetch(abortURL, {
      method: "PUT",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({}),
    });
    const result = await response.json();
    return result;
  } catch {
    // nop
  }
  return {};
}

export async function putEmergencyStop() {
  try {
    const response = await fetch(estopURL, {
      method: "PUT",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({}),
    });
    const result = await response.json();
    return result;
  } catch {
    // nop
  }
  return {};
}

export async function putHalOut(halOut: Object) {
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

export async function putLinuxCNC(command: string, data: Object) {
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

export function getHalIn(): Promise<HalIn[]> {
  return fetch(halInURL)
    .then((res) => res.json())
    .then((res) => {
      return res as HalIn[];
    });
}
