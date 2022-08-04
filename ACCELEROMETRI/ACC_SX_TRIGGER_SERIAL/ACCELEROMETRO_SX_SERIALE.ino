#include <Stepper.h>

#include <WiFi.h> /* inclusione della libreria per il WiFi */
#include <InfluxDbClient.h>   /* per libreria di comunicazione con InfluxDb */
#include <InfluxDbCloud.h>    /* per gestire il token di sicurezza di InfluxDb */
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

/* accelerometro */
Adafruit_MPU6050 mpu;

/* costanti per le connessioni */

/* OMISSIS */

#define D_TARA    100
#define D_MISURE  500

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
float perTrigger_1[10];
float perTrigger_2[5];
float AccX_Down[D_MISURE];
float AccY_Down[D_MISURE];
float AccZ_Down[D_MISURE];
float AccX_Up[D_MISURE];
float AccY_Up[D_MISURE];
float AccZ_Up[D_MISURE];

float aX, aY, aZ; /* valori accelerazione */

/* contatori */
int cTara = 0,
    c1 = 0,
    c2 = 0,
    cTempoDiscesa = 0,
    cTempoSalita = 0;

bool flgTaraInCorso = true;
bool flgUnrackAvvenuto = false;
bool flgInizioDiscesa = false;
bool flgInizioSalita = false;

/* INIZIO LOOP */
void loop() {

  readSensors();

  /* taro i sensori in base alla loro posizione */
  if (flgTaraInCorso) {

    impostaTara();

  } else {

    /* check se è avvenuto l'unrack */
    if (!flgUnrackAvvenuto) {
      flgUnrackAvvenuto = triggerUnrack();
    }

    /* quando avviene l'unrack */
    if (flgUnrackAvvenuto) {

      /* passo alla raccolta dei dati buoni */
      /* controllo se inizia la fase di discesa */

      if (!flgInizioDiscesa) {
        flgInizioDiscesa = triggerInizioDiscesa();
      }

      /* raccolgo i dati della discesa */
      if (flgInizioDiscesa && (!flgInizioSalita)) {

        AccX_Down[cTempoDiscesa] = aX;
        AccY_Down[cTempoDiscesa] = aY;
        AccZ_Down[cTempoDiscesa] = aZ;

        cTempoDiscesa++;

        /* controllo se inizia la fase di stop e risalita */

      }

    }

  }

  //writeToInfluxDb();

  delay(10);

}

/* funzione che verifica se è avvenuto l'unrack del bilanciere */
bool triggerUnrack() {

  bool ret = false;

  /* controllo se nell'ultimo decimo di secondo c'è stato un aumento dell'accelerazione ==> sollevamento del bilanciere */
  if (c1 < 10) {

    Serial.println(c1);
    perTrigger_1[c1] = aZ;  /* scrivo nell'array */

  } else {

    /* azzero il contatore dell'perTrigger_1 */
    c1 = 0;

    /* calcolo l'accelerazione media dell'ultimo decimo di secondo */
    int accMediaZ = 0;

    for (byte i = 0; i < 10; i++) {
      Serial.println(perTrigger_1[i]);
      accMediaZ += perTrigger_1[i];
    }

    accMediaZ = accMediaZ / 10;
    Serial.println("Accelerazione media nell'ultimo decimo di secondo: " + String(accMediaZ, 2));

    if (accMediaZ > 0.5) {
      Serial.println("Unrack avvenuto");
      Serial.println("");
      ret = true;
    }

  }

  /* se sto ancora popolando l'array trigger */
  if (!ret) {
    c1++;
  }

  return (ret);

}

bool triggerInizioDiscesa() {

  bool ret = false;

  /* controllo se negli ultimi 5 centesimi di secondo c'è stata una diminuzione dell'accelerazione ==> inizio fase di discesa del bilanciere */
  if (c2 < 5) {

    Serial.println(c2);
    perTrigger_2[c2] = aZ;  /* scrivo nell'array */

  } else {

    /* azzero il contatore del perTrigger_2 */
    c2 = 0;

    /* calcolo l'accelerazione media negli ultimi 5'' */
    int accMediaZ = 0;

    for (byte i = 0; i < 5; i++) {
      Serial.println(perTrigger_2[i]);
      accMediaZ += perTrigger_2[i];
    }

    accMediaZ = accMediaZ / 5;
    Serial.println("Accelerazione media negli ultimi 5'': " + String(accMediaZ, 2));

    if (accMediaZ < -0.5) {
      Serial.println("Inizio fase di discesa");
      Serial.println("");

      ret = true;
    }

  }

  /* se sto ancora popolando l'array */
  if (!ret) {
    c2++;
  }

  return (ret);

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

/* funzione che legge dal sensore */
void readSensors() {

  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  aX = a.acceleration.x - taraX;
  aY = a.acceleration.y - taraY;
  aZ = a.acceleration.z - taraZ;

  /* Print out the values */
  Serial.print("Acceleration X: ");
  Serial.print(aX);

  Serial.print(", Y: ");
  Serial.print(aY);

  Serial.print(", Z: ");
  Serial.print(aZ);

  Serial.println(" m/s^2");

}

void writeToInfluxDb() {

  /*
    sensor.clearFields();
    sensor.addField("temperature", t);
    sensor.addField("humidity", h);
    Serial.print("Writing: ");
    Serial.println(sensor.toLineProtocol());

    if(!client.writePoint(sensor)){
    Serial.print("InfluxDB write failed ");
    Serial.println(client.getLastErrorMessage());
    }
  */

}
