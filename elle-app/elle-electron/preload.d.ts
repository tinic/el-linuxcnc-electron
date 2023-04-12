declare interface api {
    send: (channel:any, data:any) => void;
    receive: (channel:any, func:any) => void;
}
