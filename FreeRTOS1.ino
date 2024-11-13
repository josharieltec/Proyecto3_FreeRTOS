#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <DHT_U.h>
#include <OneWire.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Definiciones y configuraciones de sensores
#define DHTTYPE DHT22
#define MQ_PIN 14
#define DHTPIN 19

// Curvas de calibración para el sensor MQ2
#define GAS_CO 1
#define GAS_SMOKE 2
float COCurve[3] = {2.3, 0.72, -0.34};
float SmokeCurve[3] = {2.3, 0.53, -0.44};
float Ro = 10;  // Valor base de calibración para el MQ2

// Configuración de conexión WiFi
String URL = "http://192.168.68.112/sensor_project/test.php";
const char* ssid = "NAME";
const char* password = "PASSWORD";

// Variables de datos
float temperature = 0;
float humidity = 0;
float co = 0;
float smoke = 0;

// Inicialización de los sensores
OneWire ds(4);
DallasTemperature sensors(&ds);
DHT_Unified dht(DHTPIN, DHTTYPE);

// Declaración de los manejadores de tareas
TaskHandle_t TaskReadTemperature;
TaskHandle_t TaskReadHumidity;
TaskHandle_t TaskReadGas;
TaskHandle_t TaskSendData;
TaskHandle_t TaskConnectWiFi;

// Funciones de medición de sensores
float MQCalibration(int mq_pin);
float MQResistanceCalculation(int raw_adc);
float MQRead(int mq_pin);
int MQGetGasPercentage(float rs_ro_ratio, int gas_id);
int MQGetPercentage(float rs_ro_ratio, float *pcurve);

// Tareas de FreeRTOS
void TaskReadTemperatureCode(void * pvParameters) {
  for (;;) {
    temperature = temp();
    vTaskDelay(30000 / portTICK_PERIOD_MS);  // Leer cada 30 segundos
  }
}

void TaskReadHumidityCode(void * pvParameters) {
  for (;;) {
    humidity = hum();
    vTaskDelay(30000 / portTICK_PERIOD_MS);  // Leer cada 30 segundos
  }
}

void TaskReadGasCode(void * pvParameters) {
  for (;;) {
    co = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_CO);
    smoke = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_SMOKE);
    vTaskDelay(30000 / portTICK_PERIOD_MS);  // Leer cada 30 segundos
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
    vTaskDelay(60000 / portTICK_PERIOD_MS);  // Enviar datos cada minuto
  }
}

void TaskConnectWiFiCode(void * pvParameters) {
  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);

      while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(500 / portTICK_PERIOD_MS);  // Esperar hasta que se conecte
      }
    }
    vTaskDelay(10000 / portTICK_PERIOD_MS);  // Revisar la conexión cada 10 segundos
  }
}

// Funciones de lectura de sensores
float temp() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

float hum() {
  sensors_event_t event;
  dht.humidity().getEvent(&event);
  return event.relative_humidity;
}

void setup() {
  Serial.begin(115200);
  Ro = MQCalibration(MQ_PIN);
  sensors.begin();
  dht.begin();
  analogReadResolution(12);

  // Crear las tareas de FreeRTOS
  xTaskCreatePinnedToCore(TaskReadTemperatureCode, "Read Temperature", 2048, NULL, 1, &TaskReadTemperature, 1);
  xTaskCreatePinnedToCore(TaskReadHumidityCode, "Read Humidity", 2048, NULL, 1, &TaskReadHumidity, 1);
  xTaskCreatePinnedToCore(TaskReadGasCode, "Read Gas", 2048, NULL, 1, &TaskReadGas, 1);
  xTaskCreatePinnedToCore(TaskSendDataCode, "Send Data", 4096, NULL, 1, &TaskSendData, 1);
  xTaskCreatePinnedToCore(TaskConnectWiFiCode, "Connect WiFi", 2048, NULL, 1, &TaskConnectWiFi, 1);
  // NUEVAS TAREAS??
}

void loop() {
  // El bucle principal queda vacío, ya que las tareas las maneja FreeRTOS
}
