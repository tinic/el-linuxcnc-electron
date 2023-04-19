let halOutURL = "http://localhost:8000/hal/hal_out";
let halInURL = "http://localhost:8000/hal/hal_in";
let linuxcncURL = "http://localhost:8001/linuxcnc/";

var userAgent = navigator.userAgent.toLowerCase();
if (userAgent.indexOf(" electron/") < 0) {
  halOutURL = "http://lathev2:8000/hal/hal_out";
  halInURL = "http://lathev2:8000/hal/hal_in";
  linuxcncURL = "http://lathev2:8001/linuxcnc/";
}

export interface HalIn {
    position_z: number;
    position_x: number;
    position_a: number;
    speed_rps: number;
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
  