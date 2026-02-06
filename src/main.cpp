#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

/* ------ PINOS ------ */
#define I2C_SDA       1
#define I2C_SCL       2
#define IO_WAKEUP     7
#define IO_ADC        8
#define IO_USBDETECT  15
#define LED_SERV      13
#define LED_WIFI      14

/* ------ WIFI ------ */
const char* ssid     = "Conan 3";
const char* password = "12345678";

/* ------ SERVIDOR TCP ------ */
const char* server_ip   = "192.168.106.135"; // IP do servidor
const uint16_t server_port = 5000;

WiFiClient client;
Adafruit_BME280 bme;

/* ------ PROTÓTIPOS ------ */
void readSensorData();
void connectWiFi();
void setupTime();
long getTimestamp();
void sendDataTCP(float t, float p, float u, float a);

void setup() 
{
  Serial.begin(115200);
  delay(1000);

  pinMode(IO_WAKEUP, OUTPUT);
  pinMode(IO_ADC, INPUT);
  pinMode(IO_USBDETECT, INPUT_PULLUP);
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_SERV, OUTPUT);
  digitalWrite(IO_WAKEUP, HIGH);

  Wire.begin(I2C_SDA, I2C_SCL);

  Serial.println("Iniciando BME280...");
  if (!bme.begin(0x76, &Wire) && !bme.begin(0x77, &Wire)) {
    Serial.println("BME280 não encontrado!");
    while (1) delay(10);
  }

  bme.setSampling(
    Adafruit_BME280::MODE_NORMAL,
    Adafruit_BME280::SAMPLING_X2,
    Adafruit_BME280::SAMPLING_X16,
    Adafruit_BME280::SAMPLING_X1,
    Adafruit_BME280::FILTER_X16,
    Adafruit_BME280::STANDBY_MS_500
  );

  connectWiFi();
  setupTime();
}

void loop() 
{
  if (digitalRead(IO_USBDETECT) == HIGH)
  {
    Serial.println("USB desconectado - baixo consumo");
    digitalWrite(IO_WAKEUP, LOW);
    readSensorData();
    delay(20000);
  }
  else
  {
    Serial.println("USB conectado");
    digitalWrite(IO_WAKEUP, HIGH);
    readSensorData();
    delay(1000);
  }
}

/* ------ LEITURA + ENVIO ------ */
void readSensorData()
{
  float temperatura = bme.readTemperature();
  float pressao     = bme.readPressure() / 100.0F;
  float umidade     = bme.readHumidity();
  float altitude    = bme.readAltitude(1013.25);

  // Serial.println("=== BME280 ===");
  // Serial.printf("T: %.2f °C\n", temperatura);
  // Serial.printf("P: %.2f hPa\n", pressao);
  // Serial.printf("U: %.2f %%\n", umidade);
  // Serial.printf("A: %.2f m\n", altitude);

  sendDataTCP(temperatura, pressao, umidade, altitude);
}

/* ------ WIFI ------ */
void connectWiFi()
{
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  digitalWrite(LED_WIFI, HIGH);
  Serial.println("\nWi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void setupTime()
{
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Sincronizando horário");
  time_t now;
  while (time(&now) < 100000) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nHorário sincronizado!");
}

long getTimestamp()
{
  time_t now;
  time(&now);
  return now;
}

/* ------ TCP + JSON ------ */
void sendDataTCP(float t, float p, float u, float a)
{
  if (!client.connected()) {
    Serial.println("Conectando ao servidor TCP...");
    if (!client.connect(server_ip, server_port)) {
      Serial.println("Falha ao conectar no servidor");
      digitalWrite(LED_SERV, LOW);
      return;
    }
    Serial.println("Conectado ao servidor TCP!");
    digitalWrite(LED_SERV, HIGH);
  }

  long timestamp = getTimestamp();

  String json =
    "{"
    "\"timestamp\":"   + String(timestamp) + ","
    "\"temperatura\":" + String(t, 2) + ","
    "\"pressao\":"     + String(p, 2) + ","
    "\"umidade\":"     + String(u, 2) + ","
    "\"altitude\":"    + String(a, 2) +
    "}";

  client.println(json);
  Serial.print("JSON enviado: ");
  Serial.println(json);
}