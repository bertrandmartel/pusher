#include "Arduino.h"
#include <Servo.h>
#include <RFduinoBLE.h>
#include <string>

#define FLASH_STORAGE 251 //Use the last page 
#define  str(x)   xstr(x)
#define  xstr(x)  #x

struct data_t
{
  char pass[20];
  uint16_t flag;
};

data_t config = { 
  "admin" ,
  0
};

Servo s1;

int led = 3;
int button = 5;

data_t *in_flash = (data_t*)ADDRESS_OF_PAGE(FLASH_STORAGE);

void write(){

    config.flag=1;
    while (!RFduinoBLE.radioActive);
    while (RFduinoBLE.radioActive);
    RFduinoBLE.end();
    flashPageErase(PAGE_FROM_ADDRESS(in_flash));

    int rc = flashWriteBlock(in_flash, &config, sizeof(config));
    if (rc == 0)
        Serial.println("Success");
    else if (rc == 1)
        Serial.println("Error - the flash page is reserved");
    else if (rc == 2)
        Serial.println("Error - the flash page is used by the sketch");

    RFduinoBLE.begin();
    in_flash = (data_t*)ADDRESS_OF_PAGE(FLASH_STORAGE);
    Serial.print("pass = ");
    Serial.println(in_flash->pass);
    Serial.print("flag = ");
    Serial.println(in_flash->flag & 0xFF);
}

void setup() {

  Serial.begin(9600);

  if (in_flash->flag==1){

    Serial.println("something already written");

    for (int i = 0; i < 20; i++){             
      config.pass[i] = in_flash->pass[i];
    }
  }
  else{
    Serial.println("nothing written");
  }

  pinMode(led, OUTPUT);

  pinMode(button, INPUT);
  
  digitalWrite(led, LOW);

  RFduinoBLE.advertisementData = "open LAB";
  
  RFduinoBLE.begin();

  write();
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

  int cmd = data[0];

  char parameters[len-1];

  if (len > 1){
      for (int i = 1; i < len; i++){
        parameters[i-1]=data[i];
      }
  }

  switch(cmd){
    case 0:
      Serial.println("push cmd received");
      Serial.println(parameters);
      Serial.println(config.pass);
      if (strcmp(parameters,config.pass)==0)
      {
        Serial.println("processing push cmd [SUCCESS]");
        digitalWrite(led, HIGH);
        s1.attach(2);
        motor_process();
        s1.detach();
        digitalWrite(led, LOW);
        char dataRet[]= "OK";
        RFduinoBLE.send(dataRet,strlen(dataRet));
      }
      else{
        Serial.println("processing push cmd [FAILURE]");
      }
      break;
    case 1:
      Serial.println("set password cmd received");
      if (strcmp(parameters,config.pass)==0)
      {
        Serial.println("processing set password cmd [SUCCESS]");
      }
      else{
        Serial.println("processing set password cmd [FAILURE]");
      }
      break;
    default:
      break;
  }
}

int main() {

  init();
  setup();
  while(1)
    loop();
  return 0;
}