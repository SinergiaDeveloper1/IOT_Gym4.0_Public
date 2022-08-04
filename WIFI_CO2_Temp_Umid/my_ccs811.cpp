#include "header.h"

#include <Wire.h>    // I2C library
#include "ccs811.h"  // CCS811 library

CCS811 ccs811(D3); // nWAKE on D3

uint16_t eco2, etvoc, errstat, raw;
int eco2_c, etvco_c;

double getEco2() { return double(eco2_c); }
double getEtvco() { return double(etvco_c); }
  
void initCcs811() {

  // Enable I2C
  Wire.begin(); 
  
  // Enable CCS811
  ccs811.set_i2cdelay(50); // Needed for ESP8266 because it doesn't handle I2C clock stretch correctly
  ccs811.begin();
  
  // Start measuring
  ccs811.start(CCS811_MODE_1SEC);
  
}

void readCcs811() {

  // add the call function for most set sensor ccs811
  ccs811.set_envdata_Celsius_percRH(getT(), getH()); 
  
  delay(1000);

  for(int i = 0; i < 5; i++) {
    
    ccs811.read(&eco2,
                &etvoc,
                &errstat,
                &raw); 
    
    delay(1000); 
  }
  
  /* controllo che i valori siano attendibili */
  if(eco2 < 8000)
    if ((eco2 < 400) || (eco2 > 430))
      eco2_c = eco2;
  
  if (etvoc < 1187)
    etvco_c = etvoc;
    
}
