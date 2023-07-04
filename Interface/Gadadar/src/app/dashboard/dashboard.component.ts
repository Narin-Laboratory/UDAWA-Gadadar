import { Input, Component, OnChanges, SimpleChanges, OnInit, OnDestroy } from '@angular/core';
import { LineChartData } from "chartist";
import { Subscription } from 'rxjs';
import { WebsocketService } from "../websocket.service";
import { DatePipe } from '@angular/common';
import { HttpClient } from '@angular/common/http';
import { Router } from '@angular/router';


@Component({
  selector: 'app-dashboard',
  templateUrl: './dashboard.component.html',
  styleUrls: ['./dashboard.component.css']
})

export class DashboardComponent implements OnInit, OnDestroy, OnChanges{
  private wsSubscription: Subscription | undefined;
  private wsDisconnectSubscription: Subscription | undefined;

  constructor(private wsService: WebsocketService, private http: HttpClient, private router: Router) {}
  
  logLoadProgress = 0;
  logLoadTS = "";
  logFileName = '';

  server = '';

  attr = {'dUsed': 0, 'dSize': 0};
  cfg = {};
  stg = {};
  deviceTelemetry = {'heap': 0};
  deviceAttributes = {};
  alarmTime = Date();
  alarmCode = 0;
  alarmMsg = {
    "0": "No alarm message.",
    "999": "The device has rebooted. Please check your device's operation condition to make sure no such problem is causing the reboot.",
    "110": "The light sensor failed to initialize; please check the module integration and wiring.",
    "111": "The light sensor measurement is abnormal; please check the module integrity.",
    "112": "The light sensor measurement is showing an extreme value; please monitor the device's operation closely.",
    "120": "The weather sensor failed to initialize; please check the module integration and wiring.",
    "121": "The weather sensor measurement is abnormal; The ambient temperature is out of range.",
    "122": "The weather sensor measurement is showing an extreme value; The ambient temperature is exceeding 40°C; please monitor the device's operation closely.",
    "123": "The weather sensor measurement is showing an extreme value; The ambient temperature is less than 17°C; please monitor the device's operation closely.",
    "124": "The weather sensor measurement is abnormal; The ambient humidity is out of range.",
    "125": "The weather sensor measurement is showing an extreme value; The ambient humidity is nearly 100%; please monitor the device's operation closely.",
    "126": "The weather sensor measurement is showing an extreme value; The ambient humidity is below 20%; please monitor the device's operation closely.",
    "127": "The weather sensor measurement is abnormal; The barometric pressure is out of range.",
    "128": "The weather sensor measurement is showing an extreme value; The barometric pressure is more than 1010hPa; please monitor the device's operation closely.",
    "129": "The weather sensor measurement is showing an extreme value; The barometric pressure is less than 100hPa; please monitor the device's operation closely."
  }
  chartLightSensor: LineChartData = {
    labels: [],
    series: [[]]
  }

  lightSensor = {};
  weatherSensor = {};

  lightSensorChartData: LineChartData = {
    labels: [],
    series: [[]]
  };
  lightSensorChartDataLabels = [];
  lightSensorChartDataSeries = [];
  lightSensorChartQuery = 'chart-light-sensor';
  lightSensorChartTitle = 'Ambient Light';

  weatherSensorChartData: LineChartData = {
    labels: [],
    series: [[], [], []]
  };
  weatherSensorChartDataLabels = [];
  weatherSensorChartDataSeriesTemp = [];
  weatherSensorChartDataSeriesHumidity = [];
  weatherSensorChartDataSeriesPressure = [];
  weatherSensorChartQuery = 'chart-weather-sensor';
  weatherSensorChartTitle = 'Weather Data';


  ngOnInit(): void {
    const datepipe: DatePipe = new DatePipe('en-US');

    this.server = this.wsService.getServerAddress();
    this.logFileName = datepipe.transform(new Date(), 'YYYY-M-d');
    this.wsDisconnectSubscription = this.wsService.connectionClosed$.subscribe(() => { // <-- Add this line
      this.router.navigate(['/login']);
    });
    this.wsSubscription = this.wsService.connect().subscribe({
      next: (msg) => {
        // Handle received messages
        if(msg['attr'] != null){
          this.attr = msg['attr'];
        }
        if(msg['cfg'] != null){
          this.cfg = msg['cfg'];
        }
        if(msg['stg'] != null){
          this.stg = msg['stg'];
        }
        if(msg['lightSensor'] != null){
          this.lightSensor = msg['lightSensor'];
        }
        if(msg['weatherSensor'] != null){
          this.weatherSensor = msg['weatherSensor'];
        }

        if(msg['alarm'] != null){
          this.alarmCode = msg['alarm'];
        }
        if(msg['devTel'] != null){
          this.deviceTelemetry = msg['devTel'];
          let formattedDate = datepipe.transform(this.deviceTelemetry['dt'] * 1000, 'YYYY-MM-dd HH:mm:ss');
          this.deviceTelemetry['dts'] = formattedDate;
  
          this.deviceTelemetry['rssi'] = Math.min(Math.max(2 * (this.deviceTelemetry['rssi'] + 100), 0), 100);
        }
      },
      error: (err) => console.log(err),
    });
  }

  ngOnDestroy(): void {
    this.wsDisconnectSubscription?.unsubscribe();
    this.wsSubscription?.unsubscribe();
  }

  ngOnChanges(changes: SimpleChanges): void {
    
  }

  changeAttr(){
    var data = {
      'cmd': 'attr'
    };
    data["itW"] = this.stg['itW'] < 60 ? 60 : this.stg['itW'];
    data["itL"] = this.stg['itL'] < 60 ? 60 : this.stg['itL'];
    this.wsService.send(data);
    
    data = {
      'cmd': 'attr'
    };
    if(this.cfg['wssid'] != ''){data['wssid'] = this.cfg['wssid'];}
    if(this.cfg['wpass'] != ''){data['wpass'] = this.cfg['wpass'];}
    if(this.cfg['htU'] != ''){data['htU'] = this.cfg['htU'];}
    if(this.cfg['htP'] != ''){data['htP'] = this.cfg['htP'];}
    data['fIoT'] = this.cfg['fIoT'];
    data['hname'] = this.cfg['hname'];
    this.wsService.send(data);
  }

  saveSettings(){
    var data = {
      'cmd': 'saveSettings'
    };
    this.wsService.send(data);
  }

  saveConfig(){
    var data = {
      'cmd': 'configSave'
    };
    this.wsService.send(data);
  }

  panic(){
    var data = {
      'cmd': 'setPanic'
    };
    this.wsService.send(data);
  }

  reboot(){
    var data = {
      'cmd': 'reboot'
    };
    this.wsService.send(data);
  }

  wsStreamSDCard(){
    /*var data = {
      'cmd': 'wsStreamSDCard',
      'fileName': '/'+this.logFileName+'.json'
    };
    this.wsService.send(data);*/

    this.lightSensorChartData = {
      labels: [],
      series: [[]]
    };
    this.weatherSensorChartData = {
      labels: [],
      series: [[]]
    };

    this.loadJsonLineFile('http://'+this.server+'/log/'+this.logFileName+'.json').subscribe(data => {
      const lines = data.split('\n');
      
      const jsonLines = lines.map(line => {
        try {
          return JSON.parse(line);
        } catch (e) {
          //console.error('Error parsing line:', line, e);
          return {};
        }
      });
      let lineCount = jsonLines.length;
      let lineCounter = 0;

      jsonLines.forEach(msg => {
        lineCounter++;
        
        if(msg['ts'] != null){
          this.logLoadProgress = (lineCounter+1) / lineCount * 100;
          this.logLoadTS = msg['ts'];
        }
        if(msg['lux'] !=null && msg['ts'] != null)
        {
          this.lightSensorChartDataLabels.push(msg['ts'] * 1000);
          this.lightSensorChartDataSeries.push(msg['lux']);
        }
        if(msg['celc'] !=null && msg['ts'] != null)
        {
          this.weatherSensorChartDataLabels.push(msg['ts'] * 1000);
          this.weatherSensorChartDataSeriesTemp.push(msg['celc']);
          this.weatherSensorChartDataSeriesHumidity.push(msg['rh']);
          this.weatherSensorChartDataSeriesPressure.push(msg['hpa']);
        }

        if(lineCounter >= lineCount){
          let buffer: LineChartData = {
            labels: [],
            series: [[], []]
          };

          buffer.labels = this.lightSensorChartDataLabels;
          buffer.series[0] = this.lightSensorChartDataSeries;
          this.lightSensorChartData = this.aggregateAverage(buffer, 15);

          buffer.labels = this.weatherSensorChartDataLabels;
          buffer.series[0] = this.weatherSensorChartDataSeriesTemp;
          buffer.series[1] = this.weatherSensorChartDataSeriesHumidity;
          this.weatherSensorChartData = this.aggregateAverage(buffer, 15);

          this.logLoadTS = "It's done! " + lineCount + " record(s)";
        }
      });

    });
  }

  aggregateAverage(lightSensorChartData: LineChartData, windowMinutes: number): LineChartData {
    const windowMilliseconds = windowMinutes * 60 * 1000; // Convert window to milliseconds
  
    // Initialize aggregatedData as an object where the keys will be the start time of each window
    // and the values will be an object with a sum array and count array for each series.
    const aggregatedData: {[key: number]: {sum: number[], count: number[]}} = {};
  
    lightSensorChartData.labels.forEach((timestamp, index) => {
      // Convert timestamp to the start time of the window it belongs to
      const windowStart = Math.floor((timestamp as number) / windowMilliseconds) * windowMilliseconds;
      
      // If this window hasn't been seen before, initialize it in aggregatedData
      if(!aggregatedData[windowStart]) {
        aggregatedData[windowStart] = {
          sum: new Array(lightSensorChartData.series.length).fill(0),
          count: new Array(lightSensorChartData.series.length).fill(0),
        };
      }
  
      // For each series, add the current value to the sum and increment the count
      lightSensorChartData.series.forEach((series, seriesIndex) => {
        aggregatedData[windowStart].sum[seriesIndex] += series[index];
        aggregatedData[windowStart].count[seriesIndex]++;
      });
    });
  
    // Convert aggregatedData into labels and series
    const newLabels: number[] = [];
    const newSeries: number[][] = [];
    for(let i = 0; i < lightSensorChartData.series.length; i++) {
      newSeries.push([]);
    }
    
    Object.entries(aggregatedData).forEach(([windowStart, data]) => {
      newLabels.push(Number(windowStart)); // Convert back to milliseconds
      data.sum.forEach((sum, seriesIndex) => {
        newSeries[seriesIndex].push(sum / data.count[seriesIndex]); // Calculate the average
      });
    });
  
    // Return new LineChartData object
    return {
      labels: newLabels,
      series: newSeries
    };
  }

  loadJsonLineFile(url: string) {
    return this.http.get(url, { responseType: 'text' });
  }
  
}
