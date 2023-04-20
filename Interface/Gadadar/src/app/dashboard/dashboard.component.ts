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

  @Input() selected: number;
  @Output() selectedChange = new EventEmitter<number>();

  @Output() chParamsChange = new EventEmitter<any>();

  @Output() cpMChange = new EventEmitter<any>();
  @Input() cpM: object;

  @Output() cp1AChange = new EventEmitter<any>();
  @Input() cp1A: object;

  @Output() cp1BChange = new EventEmitter<any>();
  @Input() cp1B: object;

  @Output() cp2AChange = new EventEmitter<any>();
  @Input() cp2A: object;

  @Output() cp2BChange = new EventEmitter<any>();
  @Input() cp2B: object;

  @Output() cp4AChange = new EventEmitter<any>();
  @Input() cp4A: object;

  @Output() cp4BChange = new EventEmitter<any>();
  @Input() cp4B: object;

  @Output() cp3AChange = new EventEmitter<any>();
  @Input() cp3A: object;

  @Output() lblChange = new EventEmitter<any>();
  @Input() lbl: object;

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
    'Multiple Time Daily',
    'Interval',
    'Environment Condition'
  ];

  channel = [1, 2, 3, 4];

  changeSelected(){
    this.selectedChange.emit(Number(this.selected));
  }
  changeState(){
    this.state['ch'+this.selected] = Number(this.state['ch'+this.selected]);
    this.stateChange.emit(this.state);
  }

  changeParams(){
    this.cpMChange.emit(this.cpM);
    this.cp1AChange.emit(this.cp1A);
    this.cp1BChange.emit(this.cp1B);
    this.cp2AChange.emit(this.cp2A);
    this.cp2BChange.emit(this.cp2B);
    this.cp4AChange.emit(this.cp4A);
    this.cp4BChange.emit(this.cp4B);
    this.cp3AChange.emit(this.cp3A);
    this.chParamsChange.emit();
  }

  savePermanentClick(){
    this.savePermanent.emit();
  }

  changeAttr(){
    this.cfgChange.emit(this.cfg);
    this.attrChange.emit(this.attr);
    this.lblChange.emit(this.lbl);
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
