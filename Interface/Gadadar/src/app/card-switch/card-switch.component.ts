import { Input, Component } from '@angular/core';

@Component({
  selector: 'app-card-switch',
  templateUrl: './card-switch.component.html',
  styleUrls: ['./card-switch.component.css']
})
export class CardSwitchComponent {
  @Input() state: number;
  @Input() switch: string;
  @Input() rlyCtrlMd: string;
  @Input() opMode: object;
  @Input() label: object;
}
