import { Component, Output, EventEmitter } from '@angular/core';
import { SelectionModel } from '@angular/cdk/collections';
import { DatePipe } from '@angular/common';
import { WebsocketService } from "./websocket.service";
import { DashboardComponent } from './dashboard/dashboard.component';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css'],
  providers: [WebsocketService]
})


export class AppComponent {
  title = 'UDAWA Gadadar';

  attr = {};
  cfg = {};
  deviceTelemetry = {};
  deviceAttributes = {};
  bme280 = {};
  pzem = {};

  selectedCh = 1;
  state = {'ch1': 0, 'ch2': 0, 'ch3': 0, 'ch4': 0};
  dtCyc = {"dtCycCh1":0,"dtCycCh2":0,"dtCycCh3":0,"dtCycCh4":0};
  dtRng = {"dtRngCh1":2,"dtRngCh2":2,"dtRngCh3":2,"dtRngCh4":2};
  dtCycFS = {"dtCycFSCh1":0,"dtCycFSCh2":0,"dtCycFSCh3":0,"dtCycFSCh4":0};
  dtRngFS = {"dtRngFSCh1":0,"dtRngFSCh2":0,"dtRngFSCh3":0,"dtRngFSCh4":0};
  rlyActDT = {"rlyActDTCh1":0,"rlyActDTCh2":0,"rlyActDTCh3":0,"rlyActDTCh4":0};
  rlyActDr = {"rlyActDrCh1":0,"rlyActDrCh2":0,"rlyActDrCh3":0,"rlyActDrCh4":0};
  rlyActIT = {"rlyActITCh1":0,"rlyActITCh2":0,"rlyActITCh3":0,"rlyActITCh4":0};
  rlyActITOn = {"rlyActITOnCh1":0,"rlyActITOnCh2":0,"rlyActITOnCh3":0,"rlyActITOnCh4":0};
  rlyCtrlMd = {"rlyCtrlMdCh1":0,"rlyCtrlMdCh2":0,"rlyCtrlMdCh3":0,"rlyCtrlMdCh4":0};

  constructor(private WebsocketService: WebsocketService) {
    WebsocketService.messages.subscribe(msg => {
      if(msg['attr'] != null){
        this.attr = msg['attr'];
      }
      if(msg['cfg'] != null){
        this.cfg = msg['cfg'];
      }
      if(msg['devTel'] != null){
        this.deviceTelemetry = msg['devTel'];
        const datepipe: DatePipe = new DatePipe('en-US')
        let formattedDate = datepipe.transform(this.deviceTelemetry['dt'] * 1000, 'YYYY-MM-dd HH:mm:ss');
        this.deviceTelemetry['dts'] =formattedDate;
      }
      if(msg['bme280'] != null){
        this.bme280 = msg['bme280'];
      }
      if(msg['pzem'] != null){
        this.pzem = msg['pzem'];
      }
      if(msg['dtCyc'] != null){
        this.dtCyc = msg['dtCyc'];
      }
      if(msg['dtRng'] != null){
        this.dtRng = msg['dtRng'];
      }
      if(msg['dtCycFS'] != null){
        this.dtCycFS = msg['dtCycFS'];
      }
      if(msg['dtRngFS'] != null){
        this.dtRngFS = msg['dtRngFS'];
      }
      if(msg['rlyActDT'] != null){
        let temp = msg['rlyActDT'];
        for(let i = 1; i <= 4; i++){
          const datepipe: DatePipe = new DatePipe('en-US')
          let formattedDate = datepipe.transform(temp["rlyActDTCh"+i], 'YYYY-MM-dd HH:mm:ss');
          temp["rlyActDTCh"+i] = formattedDate;
        }
        this.rlyActDT = temp;
      }
      if(msg['rlyActDr'] != null){
        this.rlyActDr = msg['rlyActDr'];
      }
      if(msg['rlyActIT'] != null){
        this.rlyActIT = msg['rlyActIT'];
      }
      if(msg['rlyActITOn'] != null){
        this.rlyActITOn = msg['rlyActITOn'];
      }
      if(msg['rlyCtrlMd'] != null){
        this.rlyCtrlMd = msg['rlyCtrlMd'];
      }
      for(let i = 1; i <= 4; i++){
        if(msg['ch'+i] != null){
          this.state['ch'+i] = msg['ch'+i];
        }
      }

    });
  };

  stateChange(){
    var data = {
      'cmd': 'setSwitch'
    };
    data['ch'] = 'ch' + this.selectedCh;
    data['state'] = Number(this.state['ch' + this.selectedCh]);

    this.WebsocketService.messages.next(data);
  }

  chParamsChange(){
    var data = {
      'cmd': 'attr',
    };
    data['dtCycCh'+this.selectedCh] = this.dtCyc['dtCycCh'+this.selectedCh];
    data['dtRngCh'+this.selectedCh] = this.dtRng['dtRngCh'+this.selectedCh];
    let ts = new Date(this.rlyActDT['rlyActDTCh'+this.selectedCh]);
    data['rlyActDTCh'+this.selectedCh] = ts.getTime();
    data['rlyActDrCh'+this.selectedCh] = this.rlyActDr['rlyActDrCh'+this.selectedCh];
    data['rlyActITCh'+this.selectedCh] = this.rlyActIT['rlyActITCh'+this.selectedCh];
    data['rlyACTITOnCh'+this.selectedCh] = this.rlyActITOn['rlyActITOnCh'+this.selectedCh];
    data['rlyCtrlMdCh'+this.selectedCh] = this.rlyCtrlMd['rlyCtrlMdCh'+this.selectedCh];
    this.WebsocketService.messages.next(data);
  }

  savePermanent(){
    var data = {
      'cmd': 'saveConfig'
    };
    this.WebsocketService.messages.next(data);
  }
}



