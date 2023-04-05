import { app, BrowserWindow, BrowserWindowConstructorOptions, Menu, ipcMain, screen } from "electron";
import path from "path";
import { isDev } from "./config";
import { appConfig } from "./electron-store/configuration";
import AppUpdater from "./auto-update";

async function createWindow() {
    const { width, height } = screen.getPrimaryDisplay().workAreaSize;
    const appBounds: any = appConfig.get("setting.appBounds");
    const BrowserWindowOptions: BrowserWindowConstructorOptions = {
        width: 550,
        minWidth: 550,
        height: 800,
        minHeight: 800,

        webPreferences: {
            preload: __dirname + "/preload.js",
            devTools: isDev,
        },
        show: false,
        alwaysOnTop: true,
        frame: true,
    };

    if (appBounds !== undefined && appBounds !== null) Object.assign(BrowserWindowOptions, appBounds);
    const mainWindow = new BrowserWindow(BrowserWindowOptions);

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

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.whenReady().then(async () => {
    // if dev
    if (isDev) {
        try {
            const { installExt } = await import("./installDevTool");
            await installExt();
        } catch (e) {
            console.log("Can not install extension!");
        }
    }

    createWindow();
    app.on("activate", function () {
        // On macOS it's common to re-create a window in the app when the
        // dock icon is clicked and there are no other windows open.
        if (BrowserWindow.getAllWindows().length === 0) createWindow();
    });
});

// Quit when all windows are closed, except on macOS. There, it's common
// for applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on("window-all-closed", () => {
    if (process.platform !== "darwin") {
        app.quit();
    }
});
