#ifndef HEADER_H
#define HEADER_H

  #include <Arduino.h>

  void initDht22();
  void readValueDht22();
  double getT();
  double getH();

  void initInfluxdb();
  void sendInfluxdb();
  
  void initCcs811();
  void readCcs811();
  double getEco2();
  double getEtvco();


#endif
