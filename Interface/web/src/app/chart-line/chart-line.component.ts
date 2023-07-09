import { Input, Component, OnChanges, SimpleChanges, OnInit, OnDestroy } from '@angular/core';
import { LineChart, LineChartData } from "chartist";
import { DatePipe } from '@angular/common';

export interface ChartData {
  labels: Array<any>;
  series: Array<any>;
}

@Component({
  selector: 'app-chart-line',
  templateUrl: './chart-line.component.html',
  styleUrls: ['./chart-line.component.css']
})
export class ChartLineComponent implements OnInit, OnDestroy, OnChanges { 
  @Input() chartData: LineChartData;
  @Input() chartQuery: String;
  @Input() chartTitle: String;

  ngOnInit(): void {

  };

  ngOnChanges(changes: SimpleChanges): void {
    if(changes['chartData'] && !changes['chartData'].firstChange){
      this.lineChart(this.chartQuery, this.chartData);
    }
  };
  
  ngOnDestroy(): void {
    
  };

  lineChart(query, data) {
    const responsiveOptions: any = [
      ['screen and (min-width: 641px) and (max-width: 2400px)', {
        showPoint: true,
        axisX: {
          labelInterpolationFnc: function(value) {
            const datepipe: DatePipe = new DatePipe('en-US')
            let formattedLabel = datepipe.transform(value, 'H');
            return formattedLabel;
          }
        }
      }],
      ['screen and (max-width: 640px)', {
        showLine: true,
        axisX: {
          labelInterpolationFnc: function(value) {
            const datepipe: DatePipe = new DatePipe('en-US')
            let formattedLabel = datepipe.transform(value, 'H');
            return formattedLabel;
          }
        }
      }]
    ];

    new LineChart("."+query, data, null, responsiveOptions);
  }
}
