#include "FS.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "time.h"
#include "esp_sntp.h"
#include <SparkFun_VEML7700_Arduino_Library.h>


#define FORMAT_LITTLEFS_IF_FAILED true

// WiFi Settings
const char* ssid = "X";
const char* password = "X";

// Time Settings
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
const char *time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)


// Sensor reading frequency Settings
VEML7700 lightSensor; // Create a VEML7700 object
const long interval = 10000;  // interval at which to read sensor (milliseconds)
unsigned long previousMillis = 0;  // will store last time that sensor was read
int intensityValue = 0;
const char *filePath = "/intensity.txt"; // file path where sensor readings will be stored


AsyncWebServer server(80);

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("failed to open file for appending: " + String(path));
    return;
  }
  if (file.print(message)) {
    Serial.println("message appended: " + String(message));
  } else {
    Serial.println("append failed: " + String(message));
  }
  file.close();
}

String readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return String("Error: Failed to open file");
  }

  String content = "";
  const size_t bufferSize = 128; // Adjust this value based on your available memory
  char buffer[bufferSize];

  while (file.available()) {
    size_t bytesRead = file.readBytes(buffer, bufferSize - 1);
    buffer[bytesRead] = '\0'; // Null-terminate the string
    content += buffer;
  }

  file.close();
  return content;
}

String deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    return String("deleted!");
  } else {
    return String("delete failed!");
  }
}

int fileExists(fs::FS &fs, const char *path) {
  Serial.println("Checking if file exists");
  File file = fs.open(path);
  if (!file) {
    Serial.println("File does not exist");
    return 0;
  }
  file.close();
  Serial.println("File exists");
  return 1;
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("No time available (yet)");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

String getLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("No time available (yet)");
    return String("Error: Time not set");
  }
  return String(mktime(&timeinfo));
}

// Callback function (gets called when time adjusts via NTP)
void timeavailable(struct timeval *t) {
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}

void logInitialLightIntensity() {
  Serial.println("Logging light intensity");
  intensityValue = (int) lightSensor.getLux();
  String timestamp = getLocalTime();
  String log = "[" + timestamp + "," + intensityValue + "]";
  appendFile(LittleFS, filePath, log.c_str() );
}

void logLightIntensity() {
  Serial.println("Logging light intensity");
  intensityValue = (int) lightSensor.getLux();
  String timestamp = getLocalTime();
  String log = ",[" + timestamp + "," + intensityValue + "]";
  // TODO: Append only up to X entries, implement rotation
  appendFile(LittleFS, filePath, log.c_str() );
}

String readLightIntensity() {
    return String((int) lightSensor.getLux());
  }

String processor(const String& var){
  Serial.println(var);
  if(var == "MEASURE_FROM_TEMPLATE"){
    return readFile(LittleFS, filePath);
  }
  return String();
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://code.highcharts.com/highcharts.js"></script>
  <style>
    body {
      min-width: 310px;
    	max-width: 800px;
    	height: 400px;
      margin: 0 auto;
    }
    h2 {
      font-family: Arial;
      font-size: 2.5rem;
      text-align: center;
    }
  </style>
</head>
<body>
  <h2>Sviesomatis</h2>
  <div id="chart-temperature" class="container"></div>
</body>
<script>
var chartT = new Highcharts.Chart({
  chart:{ renderTo : 'chart-temperature' },
  series: [{
    showInLegend: false,
    data: [%MEASURE_FROM_TEMPLATE%]
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    },
    series: { color: '#059e8a' }
  },
  xAxis: { type: 'datetime',
    // dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { text: 'Intensity (Lux)' }
  },
  credits: { enabled: false }
});
// setInterval(function ( ) {
//   var xhttp = new XMLHttpRequest();
//   xhttp.onreadystatechange = function() {
//     if (this.readyState == 4 && this.status == 200) {
//       var x = (new Date()).getTime(),
//           y = parseFloat(this.responseText);
//       if(chartT.series[0].data.length > 40) {
//         chartT.series[0].addPoint([x, y], true, true, true);
//       } else {
//         chartT.series[0].addPoint([x, y], true, false, true);
//       }
//     }
//   };
//   xhttp.open("GET", "/light", true);
//   xhttp.send();
// }, 1000 ) ;
</script>
</html>
)rawliteral";

void setup(){
  Serial.begin(115200);

if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }

  Wire.begin();
    if (lightSensor.begin() == false)
  {
    Serial.println("Unable to communicate with the VEML7700. Please check the wiring. Freezing...");
    while (1)
      ;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println(WiFi.localIP());
  
  sntp_set_time_sync_notification_cb(timeavailable);
  /**
   * A more convenient approach to handle TimeZones with daylightOffset
   * would be to specify a environment variable with TimeZone definition including daylight adjustmnet rules.
   * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
   */
  configTzTime(time_zone, ntpServer1, ntpServer2);

  if(!fileExists(LittleFS,filePath)){
    delay(5000);
    writeFile(LittleFS, filePath, "");
    logInitialLightIntensity();
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Requesting index.html");
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/light", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readLightIntensity().c_str());
  });
  server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readFile(LittleFS, filePath).c_str());
  });
  server.on("/destroy_history", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", deleteFile(LittleFS, filePath).c_str());
  });

  server.begin();
}
 
void loop(){
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
  previousMillis = currentMillis;
  logLightIntensity();
  }
}
