import { Component, Input, OnInit } from '@angular/core';
import { WebsocketService } from "../websocket.service";

@Component({
  selector: 'app-navbar',
  templateUrl: './navbar.component.html',
  styleUrls: ['./navbar.component.css'],
  providers: [WebsocketService]
})
export class NavbarComponent implements OnInit {

  fmVersion = '';
  name = '';

  constructor(private WebsocketService: WebsocketService) {
    WebsocketService.messages.subscribe(msg => {
      if(msg["name"] != null){
        this.name = msg["name"];
      }
      if(msg["fmVersion"] != null){
        this.fmVersion = "ver." + msg["fmVersion"];
      }
    });
  }

  ngOnInit() {
  }

}