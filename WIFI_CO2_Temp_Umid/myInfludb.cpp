#include "header.h"

#include <ESP8266WiFiMulti.h>
 
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

/* OMISSIS */

ESP8266WiFiMulti wifiMulti;
InfluxDBClient client(INFLUXDB_URL, 
                      INFLUXDB_ORG,
                      INFLUXDB_BUCKET, 
                      INFLUXDB_TOKEN, 
                      InfluxDbCloud2CACert);

Point sensor("progetto_LC");

void initInfluxdb() {
  
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  int count = 0;
  
  /* ciclo i tentativi di connessione */
  while (wifiMulti.run() != WL_CONNECTED) {
    
    delay(2000);
    
    /* dopo tre tentativi lo faccio ripartire */
    if (count > 3)
    {
      //Serial.println("Connessione fallita");
      while (1);
    }
  }
    
  sensor.addTag("host", "ESP8266_Co2");
  sensor.addTag("location", "Schieti");
  sensor.addTag("room", "palestra");
  
}

void sendInfluxdb() {
  
  sensor.clearFields();

  sensor.addField("Temperatura_Interna_1", getT());
  sensor.addField("Umidità_Interna_1", getH());
  
  if(getEtvco() > 10)
    sensor.addField("etvco", getEtvco());
  if(getEco2() > 10)
    sensor.addField("eco2", getEco2());

  /* invio il dato */
  client.writePoint(sensor);

}