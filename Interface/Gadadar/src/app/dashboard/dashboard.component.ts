import { Component } from '@angular/core';
import { WebsocketService } from "../websocket.service";


@Component({
  selector: 'app-dashboard',
  templateUrl: './dashboard.component.html',
  styleUrls: ['./dashboard.component.css'],
  providers: [WebsocketService]
})

export class DashboardComponent {
  title = 'socketrv';
  content = '';
  received = [];
  sent = [];
  celc = '';
  rh = '';
  hpa = '';
  alt = '';

  volt = '';
  amp = '';
  watt = '';
  freq = '';
  pf = '';

  constructor(private WebsocketService: WebsocketService) {
    WebsocketService.messages.subscribe(msg => {
      if(msg["celc"] != null){
        this.celc = msg["celc"];
      }
      if(msg["rh"] != null){
        this.rh = msg["rh"];
      }
      if(msg["hpa"] != null){
        this.hpa = msg["hpa"];
      }
      if(msg["alt"] != null){
        this.alt = msg["alt"];
      }

      if(msg["volt"] != null){
        this.volt = msg["volt"];
      }
      if(msg["amp"] != null){
        this.amp = msg["amp"];
      }
      if(msg["watt"] != null){
        this.watt = msg["watt"];
      }
      if(msg["freq"] != null){
        this.freq = msg["freq"];
      }
      if(msg["pf"] != null){
        this.pf = msg["pf"];
      }
    });
  }

  sendMsg() {
    let message = {
      source: '',
      content: ''
    };
    message.source = 'localhost';
    message.content = this.content;

    this.sent.push(message);
    this.WebsocketService.messages.next(message);
  }
}
