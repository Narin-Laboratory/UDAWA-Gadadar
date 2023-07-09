import { ComponentFixture, TestBed } from '@angular/core/testing';

import { CardSwitchComponent } from './card-switch.component';

describe('CardSwitchComponent', () => {
  let component: CardSwitchComponent;
  let fixture: ComponentFixture<CardSwitchComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ CardSwitchComponent ]
    })
    .compileComponents();

    fixture = TestBed.createComponent(CardSwitchComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
