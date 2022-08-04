#include <WiFi.h>             /* inclusione della libreria per il WiFi */
#include <DHT.h>              /* inclusione della libreria per il sensore */
#include <InfluxDbClient.h>   /* per libreria di comunicazione con InfluxDb */
#include <InfluxDbCloud.h>    /* per gestire il token di sicurezza di InfluxDb */

/* costanti per le connessioni */

/* OMISSIS */

/* costanti per l'accesso a DH11 */
#define DHTPIN  4
#define DHTTYPE DHT22

/* variabili globali */
InfluxDBClient client(INFLUXDB_URL, 
                      INFLUXDB_ORG, 
                      INFLUXDB_BUCKET, 
                      INFLUXDB_TOKEN, 
                      InfluxDbCloud2CACert);

/* definisco la tabella dove inserire i dati */
Point sensor("progetto_LC");
/* creo l'istanza di DHT22 */
DHT dht(DHTPIN, DHTTYPE);

/* variabili delle misure */
double t = 0.0;
double h = 0.0;

void setup() {

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
  int count = 0;

  while (WiFi.status() != WL_CONNECTED) {
    
    delay(1000);
    
    count++;

    /* dopo tre tentativi lo faccio ripartire */
    if (count > 3)
    {
      while (1);
    }

  }

  /* preparo la connessione a InfluxDb */
  sensor.addTag("host", "ESP_LC");
  sensor.addTag("location", "Schieti");
  sensor.addTag("room", "Palestra");

  client.validateConnection();
  
}

void loop() {
  
  /* controllo se sono collegato */
  if (WiFi.status() != WL_CONNECTED) {

    delay(1000);
    while(1);

  }

  readSensors();
  writeToInfluxDb();
  delay(30000);
  
}

/* funzione che legge dal sensore DHT11 */
void readSensors(){
  t = dht.readTemperature();
  h = dht.readHumidity();
}

void writeToInfluxDb() {
  
  sensor.clearFields();
  
  sensor.addField("Temperatura_Interna_2", t);
  sensor.addField("Umidità_Interna_2", h);

  client.writePoint(sensor);
  
}
