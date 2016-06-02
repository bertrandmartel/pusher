#include "Arduino.h"
#include <Servo.h>
#include <RFduinoBLE.h>

Servo s1;

int led = 3;
int button = 5;

void setup() {

  pinMode(led, OUTPUT);

  pinMode(button, INPUT);
  
  digitalWrite(led, LOW);

  RFduinoBLE.advertisementData = "open LAB";
  
  RFduinoBLE.begin();
}

void loop() {
  delay(10);
}

void motor_process()
{
  for(int angle = 0; angle < 175; angle++)
  {
    s1.write(angle);
    delay(5);
  }
  delay(500);
  
  for(int angle = 174; angle >0;angle--)
  {
    s1.write(angle);
    delay(5);
  }
}

void RFduinoBLE_onDisconnect(){
}

void RFduinoBLE_onReceive(char *data, int len){

  if (strcmp(data,"bubulle")==0)
  {
    digitalWrite(led, HIGH);
    s1.attach(2);
    motor_process();
    s1.detach();
    digitalWrite(led, LOW);
    char dataRet[]= "La porte s'ouvre...";
    RFduinoBLE.send(dataRet,strlen(dataRet));
  }
}

int main() {

  init();
  setup();
  while(1)
    loop();
  return 0;
}
