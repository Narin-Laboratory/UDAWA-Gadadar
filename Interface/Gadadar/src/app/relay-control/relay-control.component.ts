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

  changeState(){
    this.stateChange.emit(Number(this.state));
  }

}
