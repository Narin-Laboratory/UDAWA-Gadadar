import { Component } from '@angular/core';
import { WebsocketService } from "../websocket.service";
import { RelayChannel } from '../relay-channel';


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

  ch1 = new RelayChannel(1, 0, 0, 0, 0, 0, 0, 0, 0);
  ch2 = new RelayChannel(2, 0, 0, 0, 0, 0, 0, 0, 0);
  ch3 = new RelayChannel(3, 0, 0, 0, 0, 0, 0, 0, 0);
  ch4 = new RelayChannel(4, 0, 0, 0, 0, 0, 0, 0, 0);
  ch = [this.ch1, this.ch2, this.ch3, this.ch4];
  selectedCh: number = 1;

  opMode = [
    {'id': 0, mode: 'Manual Switch'},
    {'id': 1, mode: 'Duty Cycle'},
    {'id': 2, mode: 'Datetime'},
    {'id': 3, mode: 'Time Daily'},
    {'id': 4, mode: 'Interval'},
    {'id': 5, mode: 'Environment Condition'}
  ];


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

      for(let i = 1; i <= 4; i++){
        if(msg["rlyCtrlMdCh"+i] != null){
          this.ch[i-1].rlyCtrlMd = msg["rlyCtrlMdCh"+i];
        }
        if(msg["dtCycCh"+i] != null){
          this.ch[i-1].dtCyc = msg["dtCycCh"+i];
        }
        if(msg["dtRngCh"+i] != null){
          this.ch[i-1].dtRng = msg["dtRngCh"+i];
        }
        if(msg["rlyActDTCh"+i] != null){
          this.ch[i-1].rlyActDT = msg["rlyActDTCh"+i];
        }
        if(msg["rlyActDrCh"+i] != null){
          this.ch[i-1].rlyActDr = msg["rlyActDrCh"+i];
        }
        if(msg["rlyActITCh"+i] != null){
          this.ch[i-1].rlyActIT = msg["rlyActITCh"+i];
        }
        if(msg["ch"+i] != null){
          this.ch[i-1].state = msg["ch"+i];
        }
      }
    });
  }

  sendMsg() {
    this.sent.push(this.ch[this.selectedCh-1]);
    this.WebsocketService.messages.next(this.ch[this.selectedCh-1]);
  }
}
