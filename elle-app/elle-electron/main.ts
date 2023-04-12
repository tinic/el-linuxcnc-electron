import { app, BrowserWindow, BrowserWindowConstructorOptions, ipcMain, screen } from "electron";
import path from "path";
import fs from 'fs';
import { isDev } from "./config";
import { appConfig } from "./electron-store/configuration";
import AppUpdater from "./auto-update";

const { spawn } = require('node:child_process');

let mainWindow:BrowserWindow;

var halrun:any = null;
var halstop:any = null;

async function createWindow() {
    const { width, height } = screen.getPrimaryDisplay().workAreaSize;
    const appBounds: any = appConfig.get("setting.appBounds");
    const BrowserWindowOptions: BrowserWindowConstructorOptions = {
        width: 1366,
        minWidth: 1366,
        height: 768,
        minHeight: 768,
        webPreferences: {
            preload: __dirname + "/preload.js",
            devTools: isDev,
            nodeIntegration: false,
            contextIsolation: true,
        },
        show: false,
        alwaysOnTop: true,
        frame: true,
        fullscreen: true,
    };

    if (appBounds !== undefined && appBounds !== null) Object.assign(BrowserWindowOptions, appBounds);
    mainWindow = new BrowserWindow(BrowserWindowOptions);

    // auto updated
    if (!isDev) AppUpdater();

    // Remove menu bar
    mainWindow.removeMenu();

    // and load the index.html of the app.
    // win.loadFile("index.html");
    await mainWindow.loadURL(isDev ? "http://localhost:3000" : `file://${path.join(__dirname, "./index.html")}`);
 
    if (appBounds !== undefined && appBounds !== null && appBounds.width > width && appBounds.height > height) mainWindow.maximize();
    else mainWindow.show();


    // this will turn off always on top after opening the application
    setTimeout(() => {
        mainWindow.setAlwaysOnTop(false);
    }, 1000);

    // Open the DevTools.
    if (isDev) {
        mainWindow.webContents.openDevTools();
    }

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

app.whenReady().then(async () => {
    if (isDev) {
        try {
            const { installExt } = await import("./installDevTool");
            await installExt();
        } catch (e) {
            console.log("Can not install extension!");
        }
    }

    createWindow();
});

function stopHAL() {
    if (halrun == null) {
        return;
    }
    let hal_path = process.cwd() + "/elle-hal";
    try {
        let env = process.env; env.PATH += ":" + hal_path;
        halstop = spawn('unbuffer', ['halrun','-U'], { cwd: process.cwd() + "/elle-hal", env: env });
        halstop.stdout.on('data', (stdout:Buffer) => {
            mainWindow.webContents.send('halStdout', stdout.toString());
        });
        halstop.stderr.on('data', (stderr:Buffer) => {
            mainWindow.webContents.send('halStdout', stderr.toString());
        });
        halstop.on('close', (code:any) => {
        });
        mainWindow.webContents.send('halStopped');
        halrun = null;
    } catch {
    }
}

app.on("window-all-closed", () => {
    stopHAL();
    app.quit();
}); 

ipcMain.on('startHAL', () => {
    let hal_path = process.cwd() + "/elle-hal";
    let halfile_path = process.cwd() + "/elle-hal/lathe.hal";
    if (fs.existsSync(halfile_path)) {
        try {
            let env = process.env; env.PATH += ":" + hal_path;
            halrun = spawn('unbuffer', ['halrun', 'lathe.hal'], { cwd: hal_path, env: env});
            halrun.stdout.on('data', (stdout:Buffer) => {
                mainWindow.webContents.send('halStdout', stdout.toString());
                if (stdout.toString().startsWith("Python REST service ready!")) {
                    mainWindow.webContents.send('halStarted');
                }
            });
            halrun.stderr.on('data', (stderr:Buffer) => {
                mainWindow.webContents.send('halStdout', stderr.toString());
            });
            halrun.on('close', (code:any) => {
                mainWindow.webContents.send('halStopped');
            });
        } catch {
        }
    }
});

ipcMain.on('stopHAL', () => {
    stopHAL();
});

ipcMain.on('quit', () => {
    stopHAL();
    app.quit();
});
