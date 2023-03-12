import { Input, Component } from '@angular/core';

@Component({
  selector: 'app-card-sensor',
  templateUrl: './card-sensor.component.html',
  styleUrls: ['./card-sensor.component.css']
})
export class CardSensorComponent {
  @Input() header: string;
  @Input() value: string;
  @Input() unit: string;
  @Input() icon: string;
}
