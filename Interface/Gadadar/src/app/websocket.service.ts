import { Injectable } from "@angular/core";
import { Observable, Observer } from 'rxjs';
import { AnonymousSubject } from 'rxjs/internal/Subject';
import { Subject } from 'rxjs';
import { map } from 'rxjs/operators';
import ReconnectingWebSocket from 'reconnecting-websocket';

const UDAWA_WS = "ws://" + window.location.hostname + "/ws";
//const UDAWA_WS = "ws://" + "domain.local" + "/ws";

export interface Message {}

@Injectable({providedIn: 'root'})
export class WebsocketService {
    private subject: AnonymousSubject<MessageEvent>;
    public messages: Subject<Message>;

    constructor() {
        this.messages = <Subject<Message>>this.connect(UDAWA_WS).pipe(
            map(
                (response: MessageEvent): Message => {
                    console.log(response.data);
                    let data = JSON.parse(response.data)
                    return data;
                }
            )
        );
    }

    public connect(url): AnonymousSubject<MessageEvent> {
        if (!this.subject) {
            this.subject = this.create(url);
            console.log("Successfully connected: " + url);
        }
        return this.subject;
    }

    private create(url): AnonymousSubject<MessageEvent> {
        const options = {
            connectionTimeout: 1000,
            maxRetries: 1000000
        };
        let ws = new ReconnectingWebSocket(url, [], options);
        let observable = new Observable((obs: Observer<MessageEvent>) => {
            ws.onmessage = obs.next.bind(obs);
            ws.onerror = obs.error.bind(obs);
            ws.onclose = obs.complete.bind(obs);
            ws.onclose = () => {
                console.log("trying to reconnect");
                this.subject = null;
                this.connect(url);
            }
            return ws.close.bind(ws);
        });
        let observer = {
            error: null,
            complete: null,
            next: (data: Object) => {
                if (ws.readyState === WebSocket.OPEN) {
                    ws.send(JSON.stringify(data));
                    console.log('Message sent to websocket: ', data);
                }
            }
        };
        return new AnonymousSubject<MessageEvent>(observer, observable);
    }
}
