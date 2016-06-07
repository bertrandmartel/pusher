#include "Arduino.h"
#include <Servo.h>
#include <RFduinoBLE.h>
#include <string>

#include <AES.h>

#define E(x,y) x = y,
enum STATE_ENUM {
#include "state.h"
};

#define E(x,y) #x,
static const char *STATE_STRING_ENUM[] = {
#include "state.h"
};

uint8_t state = STATE_NONE;
uint8_t state_count= 0;
uint8_t cmd = 0;
uint16_t data_length = 0;

AES aes ;

byte key[] = 
{
  0xF2, 0x1E, 0x07, 0x8C, 0x96, 0x99, 0x5E, 0xF7, 0xED, 0xF0, 0x91, 0x84, 0x06, 0x06, 0xF3, 0x94,
  0x59, 0x90, 0x66, 0x63, 0x81, 0xE9, 0x14, 0x3E, 0x7B, 0x02, 0x7E, 0x08, 0xB6, 0xC7, 0x06, 0x26
} ;

byte my_iv[] = 
{
  0xC3, 0x78, 0x7E, 0x76, 0x31, 0x6D, 0x6B, 0x5B, 0xB8, 0x8E, 0xDA, 0x03, 0x82, 0xEB, 0x57, 0xBD
} ;
byte default_pass[64] = { 
  0x61, 0x64, 0x6d, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
uint8_t* payload;
uint8_t offset = 0;

void prekey (int bits, int blocks)
{
  byte cipher [4*N_BLOCK] ;
  byte check [4*N_BLOCK] ;
  byte iv [N_BLOCK] ;
  
  byte succ = aes.set_key (key, bits) ;

  int t0 = micros () ;
  if (blocks == 1)
    succ = aes.encrypt (default_pass, cipher) ;
  else
  {
    for (byte i = 0 ; i < 16 ; i++){
      iv[i] = my_iv[i] ;
    }
    succ = aes.cbc_encrypt (default_pass, cipher, blocks, iv) ;
  }
  int t1 = micros () - t0 ;
  Serial.print ("encrypt ") ; Serial.print ((int) succ) ;
  Serial.print (" took ") ; Serial.print (t1) ; Serial.println ("us") ;
  
  for (byte ph = 0; ph < 64; ph++){
    Serial.print (cipher[ph]>>4, HEX) ; Serial.print (cipher[ph]&15, HEX) ; Serial.print (" ") ;
  }
  
  t0 = micros () ;
  if (blocks == 1)
    succ = aes.decrypt (cipher, default_pass) ;
  else
  {
    for (byte i = 0 ; i < 16 ; i++)
      iv[i] = my_iv[i] ;
    succ = aes.cbc_decrypt (cipher, check, blocks, iv) ;
  }
  t1 = micros () - t0 ;
  Serial.print ("decrypt ") ; Serial.print ((int) succ) ;
  Serial.print (" took ") ; Serial.print (t1) ; Serial.println ("us") ;

  for (byte ph = 0; ph < 64; ph++){
    Serial.print (check[ph]>>4, HEX) ; Serial.print (check[ph]&15, HEX) ; Serial.print (" ") ;
  }
}

#define FLASH_STORAGE 251 //Use the last page 
#define  str(x)   xstr(x)
#define  xstr(x)  #x

struct data_t
{
  char pass[4*N_BLOCK];
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
  
  int blocks = 4;
  byte cipher [4*N_BLOCK] ;
  byte succ = 0;
  byte iv [N_BLOCK];

  if (blocks == 1){
    succ = aes.encrypt (default_pass, cipher) ;
  }
  else {

    for (byte i = 0 ; i < N_BLOCK ; i++){
      iv[i] = my_iv[i] ;
    }
    succ = aes.cbc_encrypt (default_pass, cipher, blocks, iv) ;
  }

  for (byte i = 0; i < (4*N_BLOCK); i++){
    config.pass[i]=cipher[i];
  }

  for (byte i = 0; i < (4*N_BLOCK); i++){
    Serial.print (cipher[i]>>4, HEX) ; Serial.print (cipher[i]&15, HEX) ; Serial.print (" ") ;
  }
  Serial.println();

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

  for (byte i = 0; i < (4*N_BLOCK); i++){
    Serial.print (in_flash->pass[i]>>4, HEX) ; Serial.print (in_flash->pass[i]&15, HEX) ; Serial.print (" ") ;
  }

  Serial.println();
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

  prekey(256,4);

  byte succ = aes.set_key (key, 256);
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

void processCommand(int cmd){
  /*
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
    */
}

void RFduinoBLE_onReceive(char *data, int len){

  if (state==STATE_NONE){
    free(payload);
    payload = 0;
    offset=0;
    state = STATE_PROCESSING;
    cmd = data[0];
    data_length = data[2] + (data[1]<<8);
    payload = (uint8_t*)malloc(data_length + 1);
    memset(payload, 0, data_length + 1);

    Serial.print("cmd: ");
    Serial.println(cmd);
    Serial.print("data_length: ");
    Serial.println(data_length);
  }
  else{
    for (int i = 0; i  < len;i++){
      payload[offset+i] = data[i];
    }
    offset+=len;
    Serial.print("length: ");
    Serial.println(data_length);
    Serial.print("offset: ");
    Serial.println(offset);

    if (offset==data_length){
      Serial.println("complete");
      for (int i = 0;i < data_length;i++){
        Serial.print (payload[i]>>4, HEX) ; Serial.print (payload[i]&15, HEX) ; Serial.print (" ") ;
        if ((i!=0) && (i%15==0)){
          Serial.println("");
        }
      }

      Serial.println("");
      Serial.println("decrypting...");

      byte cipher [4*N_BLOCK] ;
      byte check [4*N_BLOCK] ;

      for (int i = 0; i < data_length;i++){
        cipher[i]=payload[i];
      }

      byte succ = 0;

      uint8_t remain = 0;

      if (data_length%16!=0){
        remain=1;
      }

      uint16_t block_num=data_length/16 + remain;

      Serial.print("number of block: ");
      Serial.println(block_num);

      byte iv[N_BLOCK];
      for (byte i = 0 ; i < 16 ; i++){
        iv[i] = my_iv[i] ;
      }

      //decrypt
      succ = aes.cbc_decrypt(cipher, check, 4, iv) ;

      for (byte ph = 0; ph < (4*16); ph++){
        Serial.print (check[ph]>>4, HEX) ; Serial.print (check[ph]&15, HEX) ; Serial.print (" ") ;
        if ((ph!=0) && (ph%15==0)){
          Serial.println("");
        }
      }

      free(payload);
      payload = 0;
      state=STATE_NONE;
    }

  }
}

int main() {

  init();
  setup();
  while(1)
    loop();
  return 0;
}