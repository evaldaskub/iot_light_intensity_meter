#include "FS.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "time.h"
#include "secrets.h"
#include "esp_sntp.h"
#include <SparkFun_VEML7700_Arduino_Library.h>

#define FORMAT_LITTLEFS_IF_FAILED true

// Time Settings
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
const char *time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)

// Sensor reading frequency Settings
VEML7700 lightSensor; // Create a VEML7700 object
const long interval = 600000;  // interval at which to read sensor (milliseconds)
unsigned long previousMillis = 0;  // will store last time that sensor was read
int intensityValue = 0;

const char *filePath = "/intensity.txt"; // file path where sensor readings will be stored
const int MAX_ENTRIES = 2000; // Maximum number of entries per file

AsyncWebServer server(80);

// Function to get available heap memory
uint32_t getFreeMem() {
  return ESP.getFreeHeap();
}

// Function to get the size of a file
size_t getFileSize(fs::FS &fs, const char *path) {
  File file = fs.open(path, FILE_READ);
  if(!file || file.isDirectory()){
    Serial.println("Failed to open file for reading");
    return 0;
  }
  size_t fileSize = file.size();
  file.close();
  return fileSize;
}

// Function to print memory and file size information
void printSystemInfo() {
  uint32_t freeMem = getFreeMem();
  size_t fileSize = getFileSize(LittleFS, filePath);
  
  Serial.println("System Information:");
  Serial.printf("Free Heap: %u bytes\n", freeMem);
  Serial.printf("File Size: %u bytes\n", fileSize);
}


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

  String content = "[";
  const size_t bufferSize = 128; // Adjust this value based on your available memory
  char buffer[bufferSize];

  while (file.available()) {
    size_t bytesRead = file.readBytes(buffer, bufferSize - 1);
    buffer[bytesRead] = '\0'; // Null-terminate the string
    content += buffer;
  }
  content += "]";
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

void timeavailable(struct timeval *t) {
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}

void logInitialLightIntensity() {
  Serial.println("Logging light intensity");
  intensityValue = (int) lightSensor.getLux();
  String timestamp = getLocalTime();
  // js timestamps are in ms, hence need to add 000
  String log = "[" + timestamp + "000" + "," + intensityValue + "]";
  appendFile(LittleFS, filePath, log.c_str() );
}

void logLightIntensity() {
  Serial.println("Logging light intensity");
  intensityValue = (int) lightSensor.getLux();
  String timestamp = getLocalTime();
  String newEntry = "[" + timestamp + "000" + "," + String(intensityValue) + "]";
  Serial.println("Logged value: " + newEntry);
  
  String fileContent = readFile(LittleFS, filePath);
  
  // Remove brackets from the existing content
  Serial.println("fileContent before trim: " + String(fileContent));
  fileContent.remove(0, 1);
  fileContent.remove(fileContent.length() - 1);
  Serial.println("fileContent after trim: " + String(fileContent));
  
  // Prepend the new entry
  fileContent = newEntry + (fileContent.length() > 0 ? "," + fileContent : "");
  Serial.println("fileContent.length: " + String(fileContent.length()));
  
  // Split the content into entries
  int commaIndex = 0;
  int entryCount = 0;
  for (int i = 0; i < fileContent.length(); i++) {
    if (fileContent.charAt(i) == ']') {
      entryCount++;
      if (entryCount >= MAX_ENTRIES) {
        commaIndex = i + 1;
        break;
      }
    }
  }
  
  Serial.println("Entries count: " + String(entryCount));
  // Truncate to MAX_ENTRIES
  if (entryCount >= MAX_ENTRIES) {
    fileContent = fileContent.substring(0, commaIndex);
  }

  writeFile(LittleFS, filePath, fileContent.c_str());
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
  <div id="chart-light-intensity" class="container"></div>
</body>
<script>
(async () => {
      const data = await fetch('/history.json').then(response => response.json());

      var chartT = new Highcharts.Chart({
      chart:{ renderTo : 'chart-light-intensity' },
      title: { text: 'Lauros Sviesomatis'},
      series: [{ showInLegend: false, data: data }],
      plotOptions: {
        line: { animation: false, dataLabels: { enabled: true } }, series: { color: '#059e8a' } },
      xAxis: { type: 'datetime' },
      yAxis: { title: { text: 'Intensity (Lux)' } },
      credits: { enabled: false }
      });
      setInterval(function ( ) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var x = (new Date()).getTime(),
              y = parseFloat(this.responseText);
          if(chartT.series[0].data.length > 40) {
            chartT.series[0].addPoint([x, y], true, true, true);
          } else {
            chartT.series[0].addPoint([x, y], true, false, true);
          }
        }
      };
      xhttp.open("GET", "/light", true);
      xhttp.send();
      }, 5000 );
  })();
</script>
</html>
)rawliteral";

void setup(){
  Serial.begin(115200);

if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }

  printSystemInfo();

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
  server.on("/history.json", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "application/json", readFile(LittleFS, filePath).c_str());
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
