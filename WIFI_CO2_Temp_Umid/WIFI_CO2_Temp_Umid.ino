#include "header.h"

void setup() {

  initDht22();

  delay(50);
  
  initCcs811(); 
    
  initInfluxdb();

}

void loop() {
  
  readValueDht22(); 
  readCcs811();

  sendInfluxdb();

  delay(30000);
  
}
