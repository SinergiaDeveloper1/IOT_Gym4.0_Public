
#include <ArduinoMqttClient.h>
#include <ESP8266WiFi.h>
#include <DHT.h>              

#include "arduino_secrets.h"

/* costanti per l'accesso a DH11 */
#define DHTPIN  D4
#define DHTTYPE DHT11

/* user e pass WiFi presi dal secret */
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

char clientID[]  = "DATI_FINESTRA";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[]      = "192.168.0.150";
int        port          = 1883;
const char topic[]       = "iot/message";

/* creo l'istanza di DHT11 */
DHT dht(DHTPIN, DHTTYPE);

/* variabili delle misure */
double t = 0.0;
double h = 0.0;
String messaggio;

void setup() {
  
  dht.begin();

  int count = 0;
  
  WiFi.begin(ssid, pass);

  delay(500);

    while (WiFi.status() != WL_CONNECTED) {
    
    delay(2000);
    WiFi.begin(ssid, pass);
    delay(2000);
    
    count++;

    /* dopo tre tentativi lo faccio ripartire */
    if (count > 3) {
      //ESP.deepSleep(30e6);
      while(1);
    }
    
  }
    
  mqttClient.setId(clientID);
  
  if (!mqttClient.connect(broker, port)) {
    /* se non si riesce a connettere al broker, lo mando in deep sleep */
    /* se funzionasse. Ma visto che non funziona, uso il watchdog */
    //ESP.deepSleep(30e6);
    while(1);
  }

  delay(100);

  mqttClient.poll();

  t = dht.readTemperature();
  h = dht.readHumidity();

  messaggio = "{\"Temperatura_Finestra\":" + String(t, 2) + ", \"Umidità_Finestra\":" + String(h, 2) + "}";

  delay(100);

  mqttClient.beginMessage(topic);
  mqttClient.print(messaggio);
  mqttClient.endMessage();

  delay(2000);

  /* 60 secondi di Deep Sleep */
  ESP.deepSleep(60e6);
  
}

void loop() {
   
}
