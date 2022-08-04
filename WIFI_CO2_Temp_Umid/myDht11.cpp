#include "header.h"

#include <DHT.h>
#define DHTPIN D5 
#define DHTTYPE DHT22
  
DHT dht(DHTPIN, DHTTYPE);

double offset_t = 0.0;
double t = 0.0;
double h = 0.0;

double getT() { return t; } 
double getH() { return h; }


void initDht22() { 
  
  dht.begin(); 
  delay(300);
  
}

void readValueDht22() {
  
  delay(300);
  
  for(int i = 0; i < 3; i++) { 
    
    t = dht.readTemperature() + offset_t;
    h = dht.readHumidity();
  
    delay(600);
    
  }
  
}
