import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';

import { AppComponent } from './app.component';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import { FormsModule } from '@angular/forms';
import { DashboardComponent } from './dashboard/dashboard.component';
import { CardSensorComponent } from './card-sensor/card-sensor.component';
import { CardSwitchComponent } from './card-switch/card-switch.component';
import { WebsocketService } from './websocket.service';


@NgModule({
  declarations: [
    AppComponent,
    DashboardComponent,
    CardSensorComponent,
    CardSwitchComponent
  ],
  imports: [
    BrowserModule,
    BrowserAnimationsModule,
    FormsModule
  ],
  providers: [WebsocketService],
  bootstrap: [AppComponent]
})
export class AppModule { }