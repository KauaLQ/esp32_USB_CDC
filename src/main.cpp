#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <Preferences.h>
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
Preferences prefs;

#define QUEUE_SIZE 1000
struct Sample {
  long  ts;
  float t;
  float p;
  float u;
  float a;
};

/* ------ PROTÓTIPOS ------ */
void readSensorData();
void connectWiFi();
void setupTime();
long getTimestamp();
bool sendWithAck(long ts, float t, float p, float u, float a);
void initQueue();
bool enqueue(Sample &s);
bool peek(Sample &s);
void dequeue();
void processQueue();

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

  initQueue();
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
    while (true) {
      Serial.println("Cochilando...");
      delay(1000);
    }
  }
  else
  {
    Serial.println("USB conectado - LOG mode");
    digitalWrite(IO_WAKEUP, HIGH);
    readSensorData();
    delay(5000);
  }
}

/* ------ LEITURA + ENVIO ------ */
void readSensorData()
{
  Sample s;
  s.ts = getTimestamp();
  s.t  = bme.readTemperature();
  s.p  = bme.readPressure() / 100.0F;
  s.u  = bme.readHumidity();
  s.a  = bme.readAltitude(1013.25);

  enqueue(s);      // nunca perde a leitura
  processQueue();  // tenta enviar tudo que der
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

bool sendWithAck(long ts, float t, float p, float u, float a)
{
  if (!client.connected()) {
    Serial.println("Conectando ao servidor TCP...");
    if (!client.connect(server_ip, server_port)) {
      Serial.println("Falha ao conectar no servidor");
      digitalWrite(LED_SERV, LOW);
      return false;
    }
    Serial.println("Conectado ao servidor TCP!");
    digitalWrite(LED_SERV, HIGH);
  }

  String json =
    "{"
    "\"timestamp\":"   + String(ts) + ","
    "\"temperatura\":" + String(t, 2) + ","
    "\"pressao\":"     + String(p, 2) + ","
    "\"umidade\":"     + String(u, 2) + ","
    "\"altitude\":"    + String(a, 2) +
    "}";

  client.println(json);
  Serial.println("Enviado: " + json);

  unsigned long start = millis();
  while (millis() - start < 2000) {
    if (client.available()) {
      String resp = client.readStringUntil('\n');
      resp.trim();
      if (resp == "OK") {
        Serial.println("ACK recebido");
        return true;
      }
    }
  }

  Serial.println("Timeout sem ACK");
  return false;
}

void initQueue()
{
  prefs.begin("queue", false);

  if (!prefs.isKey("head")) {
    prefs.putUShort("head", 0);
    prefs.putUShort("tail", 0);
    prefs.putUShort("count", 0);
  }

  prefs.end();
}

bool enqueue(Sample &s)
{
  prefs.begin("queue", false);

  uint16_t head  = prefs.getUShort("head", 0);
  uint16_t tail  = prefs.getUShort("tail", 0);
  uint16_t count = prefs.getUShort("count", 0);

  if (count >= QUEUE_SIZE) {
    prefs.end();
    Serial.println("Buffer cheio! Amostra perdida.");
    return false;
  }

  char key[10];
  sprintf(key, "s%u", tail);
  prefs.putBytes(key, &s, sizeof(Sample));

  tail = (tail + 1) % QUEUE_SIZE;
  count++;

  prefs.putUShort("tail", tail);
  prefs.putUShort("count", count);
  prefs.end();

  return true;
}

bool peek(Sample &s)
{
  prefs.begin("queue", true);

  uint16_t count = prefs.getUShort("count", 0);
  if (count == 0) {
    prefs.end();
    return false;
  }

  uint16_t head = prefs.getUShort("head", 0);
  char key[10];
  sprintf(key, "s%u", head);

  prefs.getBytes(key, &s, sizeof(Sample));
  prefs.end();
  return true;
}

void dequeue()
{
  prefs.begin("queue", false);

  uint16_t head  = prefs.getUShort("head", 0);
  uint16_t count = prefs.getUShort("count", 0);

  char key[10];
  sprintf(key, "s%u", head);
  prefs.remove(key);

  head = (head + 1) % QUEUE_SIZE;
  count--;

  prefs.putUShort("head", head);
  prefs.putUShort("count", count);
  prefs.end();
}

void processQueue()
{
  Sample s;

  while (peek(s)) {
    if (sendWithAck(s.ts, s.t, s.p, s.u, s.a)) {
      dequeue();   // sucesso → remove
    } else {
      break;       // falhou → tenta depois
    }
  }
}