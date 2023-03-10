import { Component, Input, Output, EventEmitter } from '@angular/core';

@Component({
  selector: 'app-relay-control',
  templateUrl: './relay-control.component.html',
  styleUrls: ['./relay-control.component.css']
})
export class RelayControlComponent {
  @Input() mode: number;

  @Input() selectedCh: number;

  @Input() state: number;
  @Output() stateChange = new EventEmitter<number>();

  @Input() dtCyc: number;
  @Output() dtCycChange = new EventEmitter<number>();

  @Input() dtRng: number;
  @Output() dtRngChange = new EventEmitter<number>();

  @Input() rlyActDT: string;
  @Output() rlyActDTChange = new EventEmitter<string>();

  @Input() rlyActDr: number;
  @Output() rlyActDrChange = new EventEmitter<number>();

  @Input() rlyActIT: number;
  @Output() rlyActITChange = new EventEmitter<number>();

  @Input() rlyActITOn: number;
  @Output() rlyActITOnChange = new EventEmitter<number>();

  @Output("switchButton") switchButton: EventEmitter<any> = new EventEmitter();
  @Output("changeRelayParams") changeRelayParams: EventEmitter<any> = new EventEmitter();

  changeState(){
    this.stateChange.emit(Number(this.state));
    this.switchButton.emit();
  }

  changeDtCyc(){
    this.dtCycChange.emit(Number(this.dtCyc));
    this.changeRelayParams.emit();
  }

  changeDtRng(){
    this.dtRngChange.emit(Number(this.dtRng));
    this.changeRelayParams.emit();
  }

  changeRlyActDT(){
    this.rlyActDTChange.emit(String(this.rlyActDT));
    this.changeRelayParams.emit();
  }

  changeRlyActDr(){
    this.rlyActDrChange.emit(Number(this.rlyActDr));
    this.changeRelayParams.emit();
  }

  changeRlyActIT(){
    this.rlyActITChange.emit(Number(this.rlyActIT));
    this.changeRelayParams.emit();
  }

  changeRlyActITOn(){
    this.rlyActITOnChange.emit(Number(this.rlyActITOn));
    this.changeRelayParams.emit();
  }




}
