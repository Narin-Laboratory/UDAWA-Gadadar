import { Injectable } from '@angular/core';
import { webSocket, WebSocketSubject } from 'rxjs/webSocket';
import CryptoJS from 'crypto-js';
import { Subject } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class WebsocketService {
  private socket$: WebSocketSubject<any> | undefined;
  private serverAddress: string | undefined;
  private apiKey: string | undefined;

  public connectionClosed$ = new Subject<void>();

  public initialize(serverAddress: string, apiKey: string): void {
    this.serverAddress = serverAddress;
    this.apiKey = apiKey;
  }

  public connect(): WebSocketSubject<any> {
    if (!this.socket$ || this.socket$.closed) {
      if (!this.serverAddress || !this.apiKey) {
        throw new Error('Server address and API key must be initialized first');
      }

      let salt = CryptoJS.lib.WordArray.random(128/8).toString();
      let auth = CryptoJS.HmacSHA256(salt, this.apiKey).toString();

      this.socket$ = webSocket({
        url: 'ws://'+this.serverAddress+'/ws',
        openObserver: {
          next: () => {
            console.log('Connection established');
            this.socket$?.next({ salt: salt, auth: auth });
          }
        },
        closeObserver: {
          next: () => {
            console.log('Connection closed');
            this.socket$ = undefined;
            this.connectionClosed$.next();
          }
        },
      });
    }

    return this.socket$;
  }

  public send(data: any): void {
    this.socket$?.next(data);
  }

  public close(): void {
    this.socket$?.complete();
  }

  public isConnected(): boolean {
    return this.socket$ !== undefined && !this.socket$.closed;
  }

  public getServerAddress() {
    return this.serverAddress;
  }
}
