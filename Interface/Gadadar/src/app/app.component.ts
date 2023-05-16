import { Component, Output, EventEmitter } from '@angular/core';
import { SelectionModel } from '@angular/cdk/collections';
import { DatePipe } from '@angular/common';
import { WebsocketService } from "./websocket.service";
import { DashboardComponent } from './dashboard/dashboard.component';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})


export class AppComponent {
  title = 'UDAWA Gadadar';

  attr = {'dUsed': 0, 'dSize': 0};
  cfg = {};
  deviceTelemetry = {'heap': 0};
  deviceAttributes = {};
  bme280 = {};
  pzem = {};
  alarmTime = Date();
  alarmCode = 0;
  alarmMsg = {
    "0": "No alarm message.",
    "999": "The device has rebooted. Please check your device's operation condition to make sure no such problem is causing the reboot.",
    "111": "There was a problem when initializing the environmental sensor. Any feature that needs environmental data will malfunction.",
    "121": "There was a problem when initializing the power sensor. Any feature that requires power measurement will malfunction.",
    "131": "Please check the device's Internet connection so the device can update its internal time to Internet time. Avoid using any feature that requires precision timing.",
    "211": "The channel 1 switch has been running for nearly an hour. Please check your configuration to prevent resource depletion and instrument overheating.",
    "212": "The channel 2 switch has been running for nearly an hour. Please check your configuration to prevent resource depletion and instrument overheating.",
    "213": "The channel 3 switch has been running for nearly an hour. Please check your configuration to prevent resource depletion and instrument overheating.",
    "214": "The channel 3 switch has been running for nearly an hour. Please check your configuration to prevent resource depletion and instrument overheating.",
    "221": "The power sensor detects no power utilization, but the number of active switches is greater than zero. Please check all the instruments that are connected to the four channels to prevent instrument failures.",
    "222": "The channels are switched off, but the power sensor detects abnormal power utilization. Please check the relay module's operation to prevent relay failure."
  }

  selected = 1;
  state = {'ch1': 0, 'ch2': 0, 'ch3': 0, 'ch4': 0};
  cpM = {"cpM1":0,"cpM2":0,"cpM3":0,"cpM4":0};
  cp1A = {"cp1A1":0,"cp1A2":0,"cp1A3":0,"cp1A4":0};
  cp1B = {"cp1B1":2,"cp1B2":2,"cp1B3":2,"cp1B4":2};
  cp2A = {"cp2A1":0,"cp2A2":0,"cp2A3":0,"cp2A4":0};
  cp2B = {"cp2B1":0,"cp2B2":0,"cp2B3":0,"cp2B4":0};
  cp3A = {"cp3A1": "0:0:0-0","cp3A2": "0:0:0-0","cp3A3": "0:0:0-0","cp3A4": "0:0:0-0" };
  cp4A = {"cp4A1":0,"cp4A2":0,"cp4A3":0,"cp4A4":0};
  cp4B = {"cp4B1":0,"cp4B2":0,"cp4B3":0,"cp4B4":0};
  lbl = {"lbl1": "unnamed", "lbl2": "unnamed", "lbl3": "unnamed", "lbl4": "unnamed"};

  constructor(private WebsocketService: WebsocketService) {
    WebsocketService.messages.subscribe(msg => {
      //console.log(msg);
      if(msg['alarm'] != null){
        this.alarmCode = msg['alarm'];
      }
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
        this.deviceTelemetry['dts'] = formattedDate;

        this.deviceTelemetry['rssi'] = Math.min(Math.max(2 * (this.deviceTelemetry['rssi'] + 100), 0), 100);
      }
      if(msg['bme280'] != null){
        this.bme280 = msg['bme280'];
      }
      if(msg['pzem'] != null){
        this.pzem = msg['pzem'];
      }
      if(msg['cp1A'] != null){
        this.cp1A = msg['cp1A'];
      }
      if(msg['cp1B'] != null){
        this.cp1B = msg['cp1B'];
      }
      if(msg['cp2A'] != null){
        let temp = msg['cp2A'];
        for(let i = 1; i <= 4; i++){
          const datepipe: DatePipe = new DatePipe('en-US')
          let formattedDate = datepipe.transform(temp["cp2A"+i], 'YYYY-MM-dd HH:mm:ss');
          temp["cp2A"+i] = formattedDate;
        }
        this.cp2A = temp;
      }
      if(msg['cp2B'] != null){
        this.cp2B = msg['cp2B'];
      }
      if(msg['cp4A'] != null){
        this.cp4A = msg['cp4A'];
      }
      if(msg['cp4B'] != null){
        this.cp4B = msg['cp4B'];
      }
      if(msg['cpM'] != null){
        this.cpM = msg['cpM'];
      }
      if(msg['lbl'] != null){
        this.lbl = msg['lbl'];
      }
      if(msg['cp3A'] != null){
        let temp = msg['cp3A'];
          for(let k in temp){
            let item = JSON.parse(temp[k]);
            let param: string = '';
            for(let t in item){
              let c: string = `${item[t]['h']}:${item[t]['i']}:${item[t]['s']}-${item[t]['d']}`;
              if(param == ''){
                param += c;
              }
              else{
                param += ",";
                param += c;
              }
            }
            this.cp3A[k] = param;
          }
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
    data['ch'] = 'ch' + this.selected;
    data['state'] = Number(this.state['ch' + this.selected]);

    this.WebsocketService.messages.next(data);

  }

  attrChange(){
    var data = {
      'cmd': 'attr'
    };
    for(let key in this.lbl){
      data[key] = this.lbl[key];
    }
    this.WebsocketService.messages.next(data);
    data = {
      'cmd': 'attr'
    };
    data['wssid'] = this.cfg['wssid'];
    data['wpass'] = this.cfg['wpass'];
    data['htU'] = this.cfg['htU'];
    data['htP'] = this.cfg['htP'];
    data['fIoT'] = this.cfg['fIoT'];
    data['hname'] = this.cfg['hname'];
    this.WebsocketService.messages.next(data);
  }

  chParamsChange(){
    var data = {
      'cmd': 'attr',
    };

    data['cpM'+this.selected] = this.cpM['cpM'+this.selected];
    
    data['cp1A'+this.selected] = this.cp1A['cp1A'+this.selected];
    data['cp1B'+this.selected] = this.cp1B['cp1B'+this.selected];
    let ts = new Date(this.cp2A['cp2A'+this.selected]);
    data['cp2A'+this.selected] = ts.getTime();
    data['cp2B'+this.selected] = this.cp2B['cp2B'+this.selected];

    let cp3A = [];
    let _cp3A: object = this.cp3A['cp3A'+this.selected].split(',',24);
    for(let k in _cp3A){
      let z ={'h': 0, 'i': 0, 's': 0, 'd': 0};
      let a = _cp3A[k].split('-',2);
      z['d'] = a[1];
      let b: object = a[0].split(':',3);
      z['h'] = b[0]; z['i'] = b[1]; z['s'] = b[2];
      cp3A.push(z);
    }

    data['cp3A'+this.selected] = JSON.stringify(cp3A);

    data['cp4A'+this.selected] = this.cp4A['cp4A'+this.selected];
    data['cp4B'+this.selected] = this.cp4B['cp4B'+this.selected];

    this.WebsocketService.messages.next(data);
  }

  savePermanent(){
    var data = {
      'cmd': 'saveSettings'
    };
    this.WebsocketService.messages.next(data);
  }

  saveConfig(){
    var data = {
      'cmd': 'configSave'
    };
    this.WebsocketService.messages.next(data);
  }

  panic(){
    var data = {
      'cmd': 'setPanic'
    };
    this.WebsocketService.messages.next(data);
  }

  reboot(){
    var data = {
      'cmd': 'reboot'
    };
    this.WebsocketService.messages.next(data);
  }
}



