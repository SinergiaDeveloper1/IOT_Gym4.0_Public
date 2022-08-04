#include <Stepper.h>

#include <WiFi.h> /* inclusione della libreria per il WiFi */
#include <InfluxDbClient.h>   /* per libreria di comunicazione con InfluxDb */
#include <InfluxDbCloud.h>    /* per gestire il token di sicurezza di InfluxDb */
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>

/* accelerometro */
Adafruit_MPU6050 mpu;

/* costanti per le connessioni */
/* OMISSIS */

#define D_TARA    100
#define D_MISURE  40

/* tare accelerometro SX */
float taraX,
      taraY,
      taraZ;

/* variabili globali */
InfluxDBClient client(INFLUXDB_URL,
                      INFLUXDB_ORG,
                      INFLUXDB_BUCKET,
                      INFLUXDB_TOKEN,
                      InfluxDbCloud2CACert);

/* definisco la tabella influxDb dove inserire i dati */
Point sensor("progetto_LC");

void setup() {

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi ..");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  Serial.println(WiFi.localIP());
  Serial.println("Connesso");

  /* preparo la connessione a InfluxDb */
  sensor.addTag("host", "Accelerometro_SX");
  sensor.addTag("location", "Schieti");
  sensor.addTag("room", "Palestra");

  /* testo la connessione */
  if (client.validateConnection()) {
    Serial.println("Connected to influxDb");
  } else {
    Serial.println("Connection failed");
    Serial.println(client.getLastErrorMessage());
  }

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1);
  }
  Serial.println("MPU6050 Found!");

  /* imposto i range del sensore */
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("");
  delay(1000);

}

/* variabili globali di elaborazione dati */
float perTaraX[D_TARA];
float perTaraY[D_TARA];
float perTaraZ[D_TARA];
float AccX[D_MISURE];
float AccY[D_MISURE];
float AccZ[D_MISURE];

float aX, aY, aZ; /* valori accelerazione */

/* contatori */
int cTara = 0,
    cRaccoltaDati = 0;

bool flgTaraInCorso = true;
bool flgInizioDiscesa = false;
bool flgInizioSalita = false;

/* INIZIO LOOP */
void loop() {

  readSensors();

  /* taro i sensori in base alla loro posizione */
  if (flgTaraInCorso) {

    impostaTara();

  } else {

    if (cRaccoltaDati < D_MISURE) {

      /* raccolgo i dati della discesa */
      AccX[cRaccoltaDati] = aX;
      AccY[cRaccoltaDati] = aY;
      AccZ[cRaccoltaDati] = aZ;

      cRaccoltaDati++;

    } else {

      /* stampo sulla seriale i valori raccolti */
      Serial.print("Accelerazione X: ");
      
      for (byte i = 0; i < D_MISURE; i++) {    
        Serial.print(AccX[i]);
        Serial.print(" ");
      }

      Serial.println("");

      Serial.print("Accelerazione Y: ");
      
      for (byte i = 0; i < D_MISURE; i++) {   
        Serial.print(AccY[i]);
        Serial.print(" ");
      }

      Serial.println("");

      Serial.print("Accelerazione Z: ");
      for (byte i = 0; i < D_MISURE; i++) {      
        Serial.print(AccZ[i]);
        Serial.print(" ");
      }

      Serial.println("");

      /* elaboro i dati e li invio a InfluxDB */
      cRaccoltaDati = 0;
      writeToInfluxDb(elaboraDatoMedio());

    }

    delay(10);

  }

}

void impostaTara() {

  if (cTara < D_TARA) {

    perTaraX[cTara] = aX;
    perTaraY[cTara] = aY;
    perTaraZ[cTara] = aZ;

    cTara++;
    Serial.println(cTara);

  } else {

    /* calcolo la tara come valore medio di quelli registrati */
    for (byte i = 0; i < D_TARA; i++) {
      taraX += perTaraX[i];
      taraY += perTaraY[i];
      taraZ += perTaraZ[i];
    }

    taraX = taraX / D_TARA;
    taraY = taraY / D_TARA;
    taraZ = taraZ / D_TARA;

    Serial.println("Tara nell'asse X: " + String(taraX, 2));
    Serial.println("Tara nell'asse Y: " + String(taraY, 2));
    Serial.println("Tara nell'asse Z: " + String(taraZ, 2));
    Serial.println("");

    flgTaraInCorso = false;

  }

}

float elaboraDatoMedio() {

  float accelerazioneMediaTot = 0.0;
  float accX = 0.0;
  float accY = 0.0;
  float accZ = 0.0;

  /* calcolo la tara come valore medio di quelli registrati */
  for (byte i = 0; i < D_MISURE; i++) {
    accX += AccX[i];
    accY += AccY[i];
    accZ += AccZ[i];
  }

  accX /= D_MISURE;  
  accY /= D_MISURE;
  accZ /= D_MISURE;
  Serial.println("accX: " + String(accX, 4));
  Serial.println("accY: " + String(accY, 4));
  Serial.println("accZ: " + String(accZ, 4));

  accelerazioneMediaTot = sqrt((accX * accX) + (accY * accY) + (accZ * accZ));
  accelerazioneMediaTot = roundf(accelerazioneMediaTot * 10000) / 10000;
  
  Serial.println("Accelerazione bilanciere Sx: " + String(accelerazioneMediaTot, 4));

  return (accelerazioneMediaTot);

}

/* funzione che legge dal sensore */
void readSensors() {

  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  aX = a.acceleration.x - taraX;
  aY = a.acceleration.y - taraY;
  aZ = a.acceleration.z - taraZ;

  /* Print out the values */
  /*
  Serial.print("Acceleration X: ");
  Serial.print(aX);

  Serial.print(", Y: ");
  Serial.print(aY);

  Serial.print(", Z: ");
  Serial.print(aZ);

  Serial.println(" m/s^2"); 
  */

}

void writeToInfluxDb(float a) {

  sensor.clearFields();
  sensor.addField("Accelerazione Sx", a);

  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());

  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed ");
    Serial.println(client.getLastErrorMessage());
  }


}
