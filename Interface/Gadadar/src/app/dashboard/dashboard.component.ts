import { Input, Component, Output, EventEmitter } from '@angular/core';
import { CardSensorComponent } from '../card-sensor/card-sensor.component';
import { CardSwitchComponent } from '../card-switch/card-switch.component';

@Component({
  selector: 'app-dashboard',
  templateUrl: './dashboard.component.html',
  styleUrls: ['./dashboard.component.css']
})
export class DashboardComponent {
  @Input() deviceTelemetry: object;
  @Input() bme280: object;
  @Input() pzem: object;

  @Input() state: object;
  @Output() stateChange = new EventEmitter<any>();

  @Input() selectedCh: number;
  @Output() selectedChChange = new EventEmitter<number>();

  @Output() chParamsChange = new EventEmitter<any>();

  @Output() rlyCtrlMdChange = new EventEmitter<any>();
  @Input() rlyCtrlMd: object;

  @Output() dtCycChange = new EventEmitter<any>();
  @Input() dtCyc: object;

  @Output() dtRngChange = new EventEmitter<any>();
  @Input() dtRng: object;

  @Output() rlyActDTChange = new EventEmitter<any>();
  @Input() rlyActDT: object;

  @Output() rlyActDrChange = new EventEmitter<any>();
  @Input() rlyActDr: object;

  @Output() rlyActITChange = new EventEmitter<any>();
  @Input() rlyActIT: object;

  @Output() rlyActITOnChange = new EventEmitter<any>();
  @Input() rlyActITOn: object;

  @Output() dtCyMTChange = new EventEmitter<any>();
  @Input() dtCyMT: object;

  @Output() labelChange = new EventEmitter<any>();
  @Input() label: object;

  @Output() cfgChange = new EventEmitter<any>();
  @Input() cfg: object;

  @Output() attrChange = new EventEmitter<any>();
  @Input() attr: object;

  @Output() savePermanent = new EventEmitter<any>();
  @Output() saveConfig = new EventEmitter<any>();
  @Output() reboot = new EventEmitter<any>();
  @Output() panic = new EventEmitter<any>();



  opMode = [
    'Manual Switch',
    'Duty Cycle',
    'Datetime',
    'Time Daily',
    'Interval',
    'Environment Condition',
    'Multiple Time Daily'
  ];

  channel = [1, 2, 3, 4];

  changeSelectedCh(){
    this.selectedChChange.emit(Number(this.selectedCh));
  }
  changeState(){
    this.state['ch'+this.selectedCh] = Number(this.state['ch'+this.selectedCh]);
    this.stateChange.emit(this.state);
  }

  changeChParams(){
    this.rlyCtrlMdChange.emit(this.rlyCtrlMd);
    this.dtCycChange.emit(this.dtCyc);
    this.dtRngChange.emit(this.dtRng);
    this.rlyActDTChange.emit(this.rlyActDT);
    this.rlyActDrChange.emit(this.rlyActDr);
    this.rlyActITChange.emit(this.rlyActIT);
    this.rlyActITOnChange.emit(this.rlyActITOn);
    this.dtCyMTChange.emit(this.dtCyMT);
    this.chParamsChange.emit();
  }

  savePermanentClick(){
    this.savePermanent.emit();
  }

  changeAttr(){
    this.cfgChange.emit(this.cfg);
    this.attrChange.emit(this.attr);
    this.labelChange.emit(this.label);
  }

  saveConfigClick(){
    this.saveConfig.emit();
  }

  rebootClick(){
    this.reboot.emit();
  }

  panicClick(){
    this.panic.emit();
  }
}
