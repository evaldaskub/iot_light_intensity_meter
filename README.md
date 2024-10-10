# iot_light_intensity_meter
Quick & Dirty Light intensity sensor based on ESP32 with a local webserver, data storage and graphing possibilities

# Software
## Setup
1. Enter SSID & Password in secrets.h
2. Validate how many maximum readings can be stored on target platform and set "MAX_ENTRIES" & "interval" values (e.g. use system information message at boot time to support calculations)
3. Upload FW to ESP32
4. Use serial monitor to ensure succesful start of ESP & note down IP address for web GUI

## Web Endpoints
- <ip> : Chart of historical data stored every 10 minutes (default) + Live display of information logged every 5s (default)
- <ip>/history : raw data stored on the microcontroller
- <ip>/history.json : json data stored on the microcontroller
- <ip>/destroy_history : delete stored data file (reboot uC after file deletion, to reinitialize neccessary data)

# Hardware Setup
**TODO**
VEML7700 - SPI wiring

## Algorithm
### Phase 1 - DONE
1. Connect to WiFi
2. Connect to Time Server
3. Log timeseries data of light intensity
4. When browser is opened: retrieve logged data
### Phase 2 - PENDING
1. When browser is opened retrieve remote switch state display on/off toggle accordingly
2. OnClick turn remote switch on or off
### Phase 3 - PENDING
1. When browser is opened retrieve automatic mode state and display a toggle
2. If automatic mode: enable and disable remote switch based on light control algorithm (TODO)