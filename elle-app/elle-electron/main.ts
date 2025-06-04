import fs from 'fs';
import path from "path";
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

import { app, BrowserWindow, BrowserWindowConstructorOptions, ipcMain, screen } from "electron";
import { isDev } from "./config.js";
import { appConfig } from "./electron-store/configuration.js";
import { spawn, spawnSync, ChildProcess } from 'node:child_process';

let mainWindow:BrowserWindow;
let halrun:ChildProcess;
let halquit:boolean = false;

async function createWindow() {
    const { width, height } = screen.getPrimaryDisplay().workAreaSize;
    const appBounds: any = (appConfig as any).get("setting.appBounds");
    const BrowserWindowOptions: BrowserWindowConstructorOptions = {
        width: 1366,
        minWidth: 1366,
        height: 768,
        minHeight: 768,
        webPreferences: {
            preload: __dirname + "/preload.js",
            devTools: false,
            nodeIntegration: false,
            contextIsolation: true,
        },
        show: false,
        alwaysOnTop: true,
        frame: false,
//        fullscreen: true,
    };

    if (appBounds !== undefined && appBounds !== null) Object.assign(BrowserWindowOptions, appBounds);
    mainWindow = new BrowserWindow(BrowserWindowOptions);

    // Remove menu bar
    mainWindow.removeMenu();

    //mainWindow.webContents.openDevTools()

    // and load the index.html of the app.
    // win.loadFile("index.html");
    await mainWindow.loadURL(isDev ? "http://localhost:3000" : `file://${path.join(__dirname, "./index.html")}`);
 
    if (appBounds !== undefined && appBounds !== null && appBounds.width > width && appBounds.height > height) mainWindow.maximize();
    else mainWindow.show();

    // this will turn off always on top after opening the application
    setTimeout(() => {
        mainWindow.setAlwaysOnTop(false);
    }, 1000);

    ipcMain.handle('versions', () => {
        return {
            node: process.versions.chrome,
            chrome: process.versions.chrome,
            electron: process.versions.electron,
            version: app.getVersion(),
            name: app.getName(),
        };
    });
}

// Silence Intel GPU dmesg noise 
//app.commandLine.appendSwitch('ignore-gpu-blacklist');
//app.commandLine.appendSwitch('disable-gpu');
//app.commandLine.appendSwitch('disable-gpu-compositing');
//app.disableHardwareAcceleration()
app.commandLine.appendSwitch("gtk-version", "3")

app.whenReady().then(async () => {
    if (isDev) {
        try {
            const { installExt } = await import("./installDevTool.js");
            await installExt();
        } catch (e) {
            console.log("Can not install extension!");
        }
    }
    createWindow();
});

function cleanMess() {
    // LinuxCNC shutdowns are never clean not matter what 
    // we try it seems. So, clean up the mess ourselves.
    let hr = spawnSync('halrun', ['-U']);
    if(!halquit) {
        mainWindow.webContents.send('halStdout', hr.stdout.toString());
        mainWindow.webContents.send('halStdout', hr.stderr.toString());
    }
    let rm = spawnSync('rm', ['/tmp/linuxcnc.lock']);
    if(!halquit) {
        mainWindow.webContents.send('halStdout', rm.stdout.toString());
        mainWindow.webContents.send('halStdout', rm.stderr.toString());
    }
}

function stopHAL(quit:boolean) {
    try {
        halquit = quit;
        if (!halrun.kill('SIGINT')) {
            if (quit) {
                app.quit();
            }
        }
    } catch {
    }
}

app.on("window-all-closed", () => {
    stopHAL(true);
}); 

ipcMain.on('stopHAL', () => {
    stopHAL(false);
});

ipcMain.on('quit', () => {
    stopHAL(true);
});

ipcMain.on('startHAL', () => {
    let hal_path = process.cwd() + "/elle-hal";
    let halfile_path = process.cwd() + "/elle-hal/lathe.hal";
    if (fs.existsSync(halfile_path)) {
        try {
            cleanMess();
            let env = process.env; env.PATH += ":" + hal_path;
            halrun = spawn('unbuffer', ['linuxcnc', 'lathe.ini'], { cwd: hal_path, env: env });
            halrun.stdout?.on('data', (stdout:Buffer) => {
                mainWindow.webContents.send('halStdout', stdout.toString());
                if (stdout.toString().startsWith("{REST_API_READY}")) {
                    mainWindow.webContents.send('halStarted');
                }
            });
            halrun.stderr?.on('data', (stderr:Buffer) => {
                mainWindow.webContents.send('halStdout', stderr.toString());
            });
            halrun.on('exit', (code:any) => {
                mainWindow.webContents.send('halStopped');
                cleanMess()
                if (halquit) {
                    app.quit();
                }
            }); 
        } catch {
        }
    }
});

