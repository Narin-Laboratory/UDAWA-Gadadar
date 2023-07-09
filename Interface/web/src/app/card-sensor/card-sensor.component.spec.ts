import { ComponentFixture, TestBed } from '@angular/core/testing';

import { CardSensorComponent } from './card-sensor.component';

describe('CardSensorComponent', () => {
  let component: CardSensorComponent;
  let fixture: ComponentFixture<CardSensorComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ CardSensorComponent ]
    })
    .compileComponents();

    fixture = TestBed.createComponent(CardSensorComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
