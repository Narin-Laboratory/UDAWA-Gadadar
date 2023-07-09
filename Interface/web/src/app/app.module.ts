import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { HttpClientModule } from '@angular/common/http';
import { Routes, RouterModule } from '@angular/router';
import { WebsocketGuard } from './websocket.guard';

import { AppComponent } from './app.component';
import { FormsModule } from '@angular/forms';
import { DashboardComponent } from './dashboard/dashboard.component';
import { CardSensorComponent } from './card-sensor/card-sensor.component';
import { WebsocketService } from './websocket.service';
import { ChartistModule } from "ng-chartist";
import { ChartLineComponent } from './chart-line/chart-line.component';
import { LoginComponent } from './login/login.component';
import { CardSwitchComponent } from './card-switch/card-switch.component';

const routes: Routes = [
  { path: '', redirectTo: '/login', pathMatch: 'full' },
  { path: 'login', component: LoginComponent },
  { path: 'dashboard', component: DashboardComponent, canActivate: [WebsocketGuard] },
];

@NgModule({
  declarations: [
    AppComponent,
    DashboardComponent,
    CardSensorComponent,
    CardSwitchComponent,
    ChartLineComponent,
    LoginComponent
  ],
  imports: [
    BrowserModule,
    FormsModule,
    ChartistModule,
    HttpClientModule,
    RouterModule.forRoot(routes)
  ],
  exports: [RouterModule],
  providers: [WebsocketService],
  bootstrap: [AppComponent]
})
export class AppModule { }