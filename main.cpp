#include "Secrets.h"
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <DHT.h>

#define DHTTYPE DHT11
#define dht_pin 15

#define AWS_IOT_PUBLISH_TOPIC "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

float h;
float t;

DHT dht(dht_pin, DHTTYPE);

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("recibiendo: ");
  Serial.println(topic);

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["mensaje"];
  Serial.println(message);
}

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Conectando a red Wi-Fi");

  while (WiFi.status()!= WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  client.setServer(AWS_IOT_ENDPOINT, 8883);

  client.setCallback(messageHandler);

  Serial.println("Connectando a AWS IoT");

  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }
  
  if (!client.connected())
  {
    Serial.println("AWS IoT Desconectado!");
    return;
  }

  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IOT Conectado!");
}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["humedad"] = h;
  doc["temperatura"] = t;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void setup() {
  Serial.begin(115200);
  connectAWS();
  dht.begin();
}

void loop() {
  h = dht.readHumidity();
  t = dht.readTemperature();

  if (isnan(h) || isnan(t))
  {
    Serial.println(F("Fallo la lectura del sensor DHT!"));
    return;
  }

  Serial.print(F("Humedad: "));
  Serial.print(h);
  Serial.print(F("% Temperatura: " ));
  Serial.print(t);
  Serial.println(F("°C "));
  
  publishMessage();
  client.loop();
  delay(1000);
}