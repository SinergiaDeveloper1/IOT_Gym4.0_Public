#include <Stepper.h>

#include <WiFi.h>           /* inclusione della libreria per il WiFi */
#include <InfluxDbClient.h> /* per libreria di comunicazione con InfluxDb */
#include <InfluxDbCloud.h>  /* per gestire il token di sicurezza di InfluxDb */
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>

/* accelerometro */
Adafruit_MPU6050 mpu;

/* costanti per le connessioni */

/* OMISSIS */

#define D_MISURE 35

/* in questa versione ho rimosso la tara sugli accelerometri */

/* variabili globali */
InfluxDBClient client(INFLUXDB_URL,
                      INFLUXDB_ORG,
                      INFLUXDB_BUCKET,
                      INFLUXDB_TOKEN,
                      InfluxDbCloud2CACert);

/* definisco la tabella influxDb dove inserire i dati */
Point sensor("progetto_LC");

TaskHandle_t Task1;
TaskHandle_t Task2;

/* variabili globali di elaborazione dati */
float AccX[D_MISURE];
float AccY[D_MISURE];
float AccZ[D_MISURE];
float Buffer_AccX[D_MISURE];
float Buffer_AccY[D_MISURE];
float Buffer_AccZ[D_MISURE];

/* valori accelerazione */
float aX, aY, aZ;   
float aMedia, aMax;

/* contatori e flag */
int cRaccoltaDati = 0;
bool flgInvia = false;
bool flgInviaBuffer = false;

void setup()
{

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }

  /* preparo la connessione a InfluxDb */
  sensor.addTag("host", "Accelerometro_DX");
  sensor.addTag("location", "Schieti");
  sensor.addTag("room", "Palestra");

  /* testo la connessione */
  client.validateConnection();

  if (!mpu.begin())
  {
    while (1);
  }

  /* imposto i range del sensore */
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  /* da qui la parte multicore: divido i due task che verranno eseguiti su un core separatamente */

  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(Task1code,   /* Task function. */
                          "Task1",     /* name of task. */
                          10000,       /* Stack size of task */
                          NULL,        /* parameter of the task */
                          1,           /* priority of the task */
                          &Task1,      /* Task handle to keep track of created task */
                          0);          /* pin task to core 0 */      

  delay(500); 

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(Task2code,   /* Task function. */
                          "Task2",     /* name of task. */
                          10000,       /* Stack size of task */
                          NULL,        /* parameter of the task */
                          1,           /* priority of the task */
                          &Task2,      /* Task handle to keep track of created task */
                          1);          /* pin task to core 1 */

  /* prima di passare al loop, aspetto 5 secondi */
  delay(5000);

}

void Task1code(void * pvParameters){

  /* loop del primo task */
  for(;;){
    
    readSensors();

    /* raccolgo i dati */
    if ((cRaccoltaDati < D_MISURE))
    {

      /* raccolgo i dati della discesa */
      AccX[cRaccoltaDati] = aX;
      AccY[cRaccoltaDati] = aY;
      AccZ[cRaccoltaDati] = aZ;

      cRaccoltaDati++;

    }
    else
    {

      /* la prima volta che entro qui segnalo che devo effettuare l'invio a InfluxDB */
      if (cRaccoltaDati == D_MISURE) {
        flgInvia = true;
      }

      /* raccolgo i dati nel buffer intanto che li invia */
      Buffer_AccX[cRaccoltaDati - D_MISURE] = aX;
      Buffer_AccY[cRaccoltaDati - D_MISURE]= aY;
      Buffer_AccZ[cRaccoltaDati - D_MISURE] = aZ;

      cRaccoltaDati++;

      /* una volta riempito anche il buffer, azzero il contatore e segnalo che lo deve inviare */
      if (cRaccoltaDati == (2 * D_MISURE)) {
        cRaccoltaDati = 0;
        flgInviaBuffer = true;
      }

    }

    delay(10);

  } 
}

void Task2code(void * pvParameters){

  /* loop del secondo task */
  for(;;){
    
    if (flgInvia || flgInviaBuffer)
    {

      aMedia = elaboraDatoMedio();
      aMax = elaboraDatoMax();

      writeToInfluxDb(aMedia, 0);
      writeToInfluxDb(aMax, 1);

      flgInvia = false;
      flgInviaBuffer = false;

    }
    
    delay(10);
    
  }
}

/* INIZIO LOOP */
void loop()
{


}

float elaboraDatoMedio()
{

  float accelerazioneMediaTot = 0.0;

  for (byte i = 0; i < D_MISURE; i++)
  {
    accelerazioneMediaTot += sqrt((AccX[i] * AccX[i]) + (AccY[i] * AccY[i]) + (AccZ[i] * AccZ[i]));
  }

  accelerazioneMediaTot /= D_MISURE;
  accelerazioneMediaTot = roundf(accelerazioneMediaTot * 10000) / 10000;

  return (accelerazioneMediaTot);

}

float elaboraDatoMax()
{

  float accelerazioneMax = 0.0;
  float perMax[D_MISURE];

  for (byte i = 0; i < D_MISURE; i++)
  {

    perMax[i] = sqrt((AccX[i] * AccX[i]) + (AccY[i] * AccY[i]) + (AccZ[i] * AccZ[i]));

    if (perMax[i] > accelerazioneMax)
    {
      accelerazioneMax = perMax[i];
    }
  }

  accelerazioneMax = roundf(accelerazioneMax * 10000) / 10000;

  return (accelerazioneMax);
}

/* funzione che legge dal sensore */
void readSensors()
{

  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  /* controllo che i valori letti siano sensati, altrimenti li scarto */
  if (a.acceleration.x < 5.0 && a.acceleration.x > -5.0) {
    aX = a.acceleration.x;
  }
  if (a.acceleration.y < 10.0 && a.acceleration.y > -10.0) {
    aY = a.acceleration.y;
  }
  if (a.acceleration.z < 16.0 && a.acceleration.z > -17.0) {
    aZ = a.acceleration.z;
  }

}

/* una volta scrivo il valore medio, una volta il max */
void writeToInfluxDb(float a, int flgMedia0_Max1)
{

  if (flgMedia0_Max1 == 0)
  {
    sensor.clearFields();
    sensor.addField("Accelerazione_Media_DX", a);
  }
  else
  {
    sensor.clearFields();
    sensor.addField("Accelerazione_Max_DX", a);
  }

  client.writePoint(sensor);

}