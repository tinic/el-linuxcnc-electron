import Store from 'electron-store';

interface AppConfig {
    setting: {
        appBounds?: any;
    };
}

export const appConfig = new Store<AppConfig>({
    name: 'appConfig',
    defaults: {
        setting: {},
    },
    schema: {
        setting: {
            type: 'object',
        },
    },
});
