declare interface api {
    send: (channel:any, data:any) => void;
    receive: (channel:any, func:any) => void;
}

declare interface settings {
    get: () => Promise<{ diameterMode: boolean; defaultMetricOnStartup: boolean; }>;
    save: (settings: { diameterMode: boolean; defaultMetricOnStartup: boolean; }) => Promise<boolean>;
}
