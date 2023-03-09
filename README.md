# UDAWA Gadadar [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.5774602.svg)](https://doi.org/10.5281/zenodo.5774602)
This variant (Gadadar) of the UDAWA Smart System is intended to help farmers automate their greenhouse instruments. The main feature of this variant is a four-channel smart relay that can be plugged into any AC instrument (pump, mixer, blower, lighting, etc.). This subsystem is also equipped with environmental sensors (air temperature, humidity, and pressure). To assist farmers in gaining insight into power usage, a precision power sensor was also added to record the real-time power usage of the controlled instruments, so farmers could see the monthly report of their power consumption.

### Key Features
1. Four-channel smart relay.
    - Manual Switch: You can switch the relay manually via the switch button on the App.
    - Duty Cycle: You can switch the relay by using a time range and duty cycle. For example, a 50% duty cycle over a 10 minute time range would give you 5 minutes on and 5 minutes off.
    - Date Time: You can switch the relay precisely at the selected datetime and adjust the on duration.
    - Time Daily: You can switch the relay on at a selected time daily and adjust the on duration.
    - Interval: You can switch the relay to on by interval and adjust the on duration.
    - Environmental Condition: You can use the environmental data as a parameter to adjust when to turn on the switch and for how long it stays on.
2. Environmental sensors.
    - View real-time data and record it for further analysis of farming conditions.
    - Use the data to control the instruments, such as misting pumps and blowers, to adjust the microclimate condition automatically.
3. Precision power sensor.
    - View real-time data and record it for a monthly report to gain insight on power efficiency.
    - Use the data to do predictive maintenance on the instruments.
    
### Built-in Interface (Non-Cloud)
After powering on the device, it will try to connect to the WiFi network. After successfully connecting to the WiFi (the LED indicator turns green or blue), you can access the device by typing the device's IP address or its local domain name in the web browser. Then you can log in by typing the authentication details; after logging in, you will see the built-in web interface to monitor or control the device locally.
![UDAWA Gadadar - Built-in web interface](Interface/Screenshot/Built-in%20Web%20Interface.png)


### Cloud Interface (MADAVA Cloud Plus)
While the built-in interface is only useful when you are on the same local network as the device, the cloud interface can help you if you are away. The cloud infrastructure uses Thingsboard Community Edition as the IoT platform. You can control or monitor the device anytime and anywhere via the Internet from your smart phone. Please note that the data logger feature only works in this mode. The standalone mode with the built-in web interface does not have recording capability due to the device's memory and resource limitations.
![UDAWA Gadadar - Cloud web interface](Interface/Screenshot/Cloud%20Web%20Interface.png)

### The Device
The hardware components are very common to anyone who is familiar with the Arduino ecosystem. The main microcontroller unit uses the ESP32 Development Board (DevkitC V4), and on the real-time control side, a small and famous development board, the Arduino Nano, is used. All of the hardware components can be easily found on the market.
![UDAWA Gadadar - PCB Design](Hardware/Rendered%20Image/UDWA%20Gadadar%20-%20PCB.jpg)
![UDAWA Gadadar - Base Plate](Hardware/Rendered%20Image/UDAWA%20Gadadar%20-%20Base%20Plate.jpg)
![UDAWA Gadadar - Box](Hardware/Rendered%20Image/UDAWA%20Gadadar%20-%20Box.jpg)
![UDAWA Gadadar - Device](Docs/img/UDAWA%20Gadadar%20-%20Device.jpg)
