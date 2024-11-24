#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <DHT_U.h>
#include <OneWire.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Sensor configurations
#define DHTTYPE DHT22
#define MQ_PIN 14
#define DHTPIN 19

// Calibration curves for the MQ2 sensor
#define GAS_CO 1
#define GAS_SMOKE 2
float COCurve[3] = {2.3, 0.72, -0.34};
float SmokeCurve[3] = {2.3, 0.53, -0.44};
float Ro = 10;  // Calibration value for the MQ2

// WiFi settings
String URL = "http://192.168.68.120/sensor_project/test.php";

// Data variables
float temperature = 0;
float humidity = 0;
float co = 0;
float smoke = 0;

// Sensor initialization
OneWire ds(4);
DallasTemperature sensors(&ds);
DHT_Unified dht(DHTPIN, DHTTYPE);

// FreeRTOS task handlers
TaskHandle_t TaskReadTemperature;
TaskHandle_t TaskReadHumidity;
TaskHandle_t TaskReadGas;
TaskHandle_t TaskSendData;
TaskHandle_t TaskConnectWiFi;

// Sensor functions
float MQCalibration(int mq_pin);
float MQResistanceCalculation(int raw_adc);
float MQRead(int mq_pin);
int MQGetGasPercentage(float rs_ro_ratio, int gas_id);
int MQGetPercentage(float rs_ro_ratio, float *pcurve);
void LoadData();

// Sensor reading functions
float temp() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

float hum() {
  sensors_event_t event;
  dht.humidity().getEvent(&event);
  return event.relative_humidity;
}

void LoadData() {
  co = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_CO);
  smoke = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_SMOKE);
}

// Calibration and reading functions
float MQCalibration(int mq_pin) {
  int i;
  float val = 0;

  for (i = 0; i < 50; i++) {  // 50 samples for calibration
    val += MQResistanceCalculation(analogRead(mq_pin));
    delay(500);
  }
  val = val / 50;
  val = val / 9.83;  // Ro in clean air
  return val;
}

float MQResistanceCalculation(int raw_adc) {
  return ((float)5 * (4095 - raw_adc) / raw_adc);
}

float MQRead(int mq_pin) {
  int i;
  float rs = 0;

  for (i = 0; i < 5; i++) {
    rs += MQResistanceCalculation(analogRead(mq_pin));
    delay(50);
  }
  rs = rs / 5;
  return rs;
}

int MQGetGasPercentage(float rs_ro_ratio, int gas_id) {
  if (gas_id == GAS_CO) {
    return MQGetPercentage(rs_ro_ratio, COCurve);
  } else if (gas_id == GAS_SMOKE) {
    return MQGetPercentage(rs_ro_ratio, SmokeCurve);
  }
  return 0;
}

int MQGetPercentage(float rs_ro_ratio, float *pcurve) {
  return (pow(10, ((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0]));
}

// FreeRTOS tasks
void TaskReadTemperatureCode(void * pvParameters) {
  for (;;) {
    temperature = temp();
    if (isnan(temperature)) temperature = 0;  // Handle invalid readings
    vTaskDelay(30000 / portTICK_PERIOD_MS);  // Read every 30 seconds
  }
}

void TaskReadHumidityCode(void * pvParameters) {
  for (;;) {
    humidity = hum();
    if (isnan(humidity)) humidity = 0;  // Handle invalid readings
    vTaskDelay(30000 / portTICK_PERIOD_MS);  // Read every 30 seconds
  }
}

void TaskReadGasCode(void * pvParameters) {
  for (;;) {
    LoadData();  // Load CO and smoke data
    vTaskDelay(30000 / portTICK_PERIOD_MS);  // Read every 30 seconds
  }
}

void TaskSendDataCode(void * pvParameters) {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      String postData = "temperature=" + String(temperature) + "&humidity=" + String(humidity) +
                        "&co=" + String(co) + "&smoke=" + String(smoke);
      HTTPClient http;
      http.begin(URL);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      int httpCode = http.POST(postData);
      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
        }
      }
      
      http.end();
    }
    vTaskDelay(60000 / portTICK_PERIOD_MS);  // Send data every minute
  }
}

void TaskConnectWiFiCode(void * pvParameters) {
  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);

      while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(500 / portTICK_PERIOD_MS);  // Wait until connected
      }
    }
    vTaskDelay(10000 / portTICK_PERIOD_MS);  // Check connection every 10 seconds
  }
}



void setup() {
  Serial.begin(115200);
  Ro = MQCalibration(MQ_PIN);
  sensors.begin();
  dht.begin();
  analogReadResolution(12);

  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(TaskReadTemperatureCode, "Read Temperature", 2048, NULL, 1, &TaskReadTemperature, 1);
  xTaskCreatePinnedToCore(TaskReadHumidityCode, "Read Humidity", 2048, NULL, 1, &TaskReadHumidity, 1);
  xTaskCreatePinnedToCore(TaskReadGasCode, "Read Gas", 2048, NULL, 1, &TaskReadGas, 1);
  xTaskCreatePinnedToCore(TaskSendDataCode, "Send Data", 4096, NULL, 1, &TaskSendData, 1);
  xTaskCreatePinnedToCore(TaskConnectWiFiCode, "Connect WiFi", 2048, NULL, 1, &TaskConnectWiFi, 1);
}

void loop() {
  // Empty, FreeRTOS handles the tasks
}