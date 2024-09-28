# iot_light_intensity_meter
Light intensity sensor based on ESP32 with a local webserver, data graphing and smart switches interface

## Algorithm
### Phase 1
1. Connect to WiFi
2. Connect to Time Server
3. Log timeseries data of light intensity
4. When browser is opened: retrieve logged data
### Phase 2
1. When browser is opened retrieve remote switch state display on/off toggle accordingly
2. OnClick turn remote switch on or off
### Phase 3
1. When browser is opened retrieve automatic mode state and display a toggle
2. If automatic mode: enable and disable remote switch based on light control algorithm (TODO)

# Acknowledgements
Arduino ESP32 examples
https://randomnerdtutorials.com/esp32-esp8266-plot-chart-web-server/

