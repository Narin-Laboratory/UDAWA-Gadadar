import { Component } from '@angular/core';
import { WebsocketService } from "../websocket.service";
import { RelayChannel } from '../relay-channel';
import { SelectionModel } from '@angular/cdk/collections';
import { DatePipe } from '@angular/common';


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

  ch1 = new RelayChannel(1, 0, 0, 0, '', 0, 0, 0, 0);
  ch2 = new RelayChannel(2, 0, 0, 0, '', 0, 0, 0, 0);
  ch3 = new RelayChannel(3, 0, 0, 0, '', 0, 0, 0, 0);
  ch4 = new RelayChannel(4, 0, 0, 0, '', 0, 0, 0, 0);
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
          const datepipe: DatePipe = new DatePipe('en-US')
          let formattedDate = datepipe.transform(msg["rlyActDTCh"+i], 'YYYY-MM-dd HH:mm:ss');
          this.ch[i-1].rlyActDT = formattedDate;
        }
        if(msg["rlyActDrCh"+i] != null){
          this.ch[i-1].rlyActDr = msg["rlyActDrCh"+i];
        }
        if(msg["rlyActITCh"+i] != null){
          this.ch[i-1].rlyActIT = msg["rlyActITCh"+i];
        }
        if(msg["rlyActITOnh"+i] != null){
          this.ch[i-1].rlyActITOn = msg["rlyActITOnCh"+i];
        }
        if(msg["ch"+i] != null){
          this.ch[i-1].state = msg["ch"+i];
        }
      }
    });
  }

  changeRelayParams(){
    var data = {
      'cmd': 'attr'
    };
    data['rlyCtrlMdCh'+this.selectedCh] = this.ch[this.selectedCh-1].rlyCtrlMd;
    data['dtCycCh'+this.selectedCh] = this.ch[this.selectedCh-1].dtCyc;
    data['dtRngCh'+this.selectedCh] = this.ch[this.selectedCh-1].dtRng;
    let ts = new Date(this.ch[this.selectedCh-1].rlyActDT);
    data['rlyActDTCh'+this.selectedCh] = ts.getTime();
    data['rlyActDrCh'+this.selectedCh] = this.ch[this.selectedCh-1].rlyActDr;
    data['rlyActITCh'+this.selectedCh] = this.ch[this.selectedCh-1].rlyActIT;
    data['rlyActITOnCh'+this.selectedCh] = this.ch[this.selectedCh-1].rlyActITOn;

    this.sent.push(data);
    this.WebsocketService.messages.next(data);
  }

  savePermanent(){
    var data = {
      'cmd': 'save'
    };
    this.sent.push(data);
    this.WebsocketService.messages.next(data);
  }

  switchButton(){
    var data = {
      'cmd': 'switch'
    };
    data['ch'] = 'ch' + this.selectedCh;
    data['state'] = this.ch[this.selectedCh-1].state;

    this.sent.push(data);
    this.WebsocketService.messages.next(data);
  }

}
