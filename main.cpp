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

#define E(x,y) x = y,
enum COMMAND_ENUM {
#include "command.h"
};

#define E(x,y) #x,
static const char *COMMAND_STRING_ENUM[] = {
#include "command.h"
};

struct device
{
  char device_id[8];
  char xor_key[32];
};

#define LFSR_DEFAULT_VALUE 0xACE1u
#define MAX_ASSOCIATED_DEVICE 25

device * device_ptr = new device[MAX_ASSOCIATED_DEVICE];
char token[16];

uint8_t state = STATE_NONE;
uint8_t state_count= 0;
uint8_t cmd = 0;
uint16_t data_length = 0;
bool use_local_keys = true;
uint16_t sending_index = 0;

AES aes ;

byte default_key[] = 
{
  0xF2, 0x1E, 0x07, 0x8C, 0x96, 0x99, 0x5E, 0xF7, 0xED, 0xF0, 0x91, 0x84, 0x06, 0x06, 0xF3, 0x94,
  0x59, 0x90, 0x66, 0x63, 0x81, 0xE9, 0x14, 0x3E, 0x7B, 0x02, 0x7E, 0x08, 0xB6, 0xC7, 0x06, 0x26
} ;

byte default_iv[] = 
{
  0xC3, 0x78, 0x7E, 0x76, 0x31, 0x6D, 0x6B, 0x5B, 0xB8, 0x8E, 0xDA, 0x03, 0x82, 0xEB, 0x57, 0xBD
} ;

byte default_pass[64] = { 
  0x61, 0x64, 0x6d, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

byte external_key[32];
byte external_iv[16];

uint8_t* payload;
uint8_t* response;
uint8_t offset = 0;

#define CONFIG_STORAGE 251
#define DEVICE_CONFIG_STORAGE 250

#define  str(x)   xstr(x)
#define  xstr(x)  #x

int interupt_pin = 31;

uint8_t myRandom8(){

  uint8_t __value;

  NRF_RNG->TASKS_START = 1; // Turn on the RNG.

  NRF_RNG->EVENTS_VALRDY = 0; // wait for a RNG value to be available

  while( NRF_RNG->EVENTS_VALRDY == 0 ){}
  __value = (uint8_t)NRF_RNG->VALUE; // read the RNG value

  NRF_RNG->TASKS_STOP = 1; // Turn off the RNG if we don't need it much.

  return(__value);
}

uint16_t myRandom16(){
  return(myRandom8() * myRandom8());
}

struct data_t
{
  char pass[4*N_BLOCK];
  uint16_t lfsr;
  uint16_t flag;
  uint8_t device_num;
  byte key[32];
  byte iv[16];
};

data_t config = { 
  "admin" ,
  0
};

Servo s1;

bool add_device_pending = false;

int led = 3;
int button = 5;

data_t *in_flash = (data_t*)ADDRESS_OF_PAGE(CONFIG_STORAGE);

uint32_t *device_config = ADDRESS_OF_PAGE(DEVICE_CONFIG_STORAGE);

uint16_t bit;

uint16_t random()
{
  bit  = ((config.lfsr >> 0) ^ (config.lfsr >> 2) ^ (config.lfsr >> 3) ^ (config.lfsr >> 5) ) & 1;
  return config.lfsr =  (config.lfsr >> 1) | (bit << 15);
}

void write_host_config(device *item){

  int rc = flashWriteBlock(device_config, item, sizeof(device));
  device_config+=10;

  if (rc == 0)
    Serial.println("Success");
  else if (rc == 1)
      Serial.println("Error - the flash page is reserved");
  else if (rc == 2)
      Serial.println("Error - the flash page is used by the sketch");
}

void save_config(bool default_config){

    Serial.println("save_config");

    if (default_config){

      byte succ = aes.set_key (default_key, 256);

      int blocks = 4;
      byte cipher [4*N_BLOCK] ;
      byte iv [N_BLOCK];

      if (blocks == 1){
        succ = aes.encrypt(default_pass, cipher) ;
      }
      else {

        for (byte i = 0 ; i < N_BLOCK ; i++){
          iv[i] = default_iv[i] ;
        }
        succ = aes.cbc_encrypt(default_pass, cipher, blocks, iv) ;
      }

      for (byte i = 0; i < (4*N_BLOCK); i++){
        config.pass[i]=cipher[i];
      }

      for (byte i = 0; i < (4*N_BLOCK); i++){
        Serial.print (cipher[i]>>4, HEX) ; Serial.print (cipher[i]&15, HEX) ; Serial.print (" ") ;
      }
      Serial.println();

      config.lfsr = LFSR_DEFAULT_VALUE;
      config.flag = 1;
      config.device_num=0;

      for (int i  =0;i< 32;i++){
        config.key[i]=default_key[i];
      }
      for (int i  =0;i< 16;i++){
        config.iv[i]=default_iv[i];
      }
    }
    
    while (!RFduinoBLE.radioActive);
    while (RFduinoBLE.radioActive);

    RFduinoBLE.end();

    flashPageErase(PAGE_FROM_ADDRESS(in_flash));

    int rc = flashWriteBlock(in_flash, &config, sizeof(config));

    if (add_device_pending){

      Serial.println("in add_device_pending");

      if (config.device_num>0){
        Serial.println("saving new associated device");
        write_host_config(&device_ptr[config.device_num-1]);
      }
    }
}

void add_device(char * device_id,char * xor_key){

  Serial.println("adding device in configuration");

  if (config.device_num<MAX_ASSOCIATED_DEVICE){

    Serial.println(config.device_num);
    memcpy(device_ptr[config.device_num].device_id, device_id,8);
    memcpy(device_ptr[config.device_num].xor_key, xor_key,32);

    Serial.println("add device to config");

    config.device_num++;

    add_device_pending=true;

    Serial.println(config.device_num);
  }
  else{
    Serial.println("error max device is reached");
  }
}

void load_fake_host_config(){

  for (int i = 0; i< MAX_ASSOCIATED_DEVICE;i++){

    if (i>=0){
      char buf[8];
      char xor_key[32];

      sprintf(buf, "d%d",i);
      sprintf(xor_key, "xor%d",i);

      strcpy(device_ptr[i].device_id, buf);
      strcpy(device_ptr[i].xor_key, xor_key);
    }
    else {
      char buf[8]={5, 21, 125, 245, 252, 14 ,86 ,59};
      char xor_key[32]={
        0x72, 0x6E , 0x35, 0x77, 0x6D, 0x64, 0x64, 0x64, 0x32, 0x62, 0x71, 0x31, 0x65, 0x64, 0x6B, 0x63, 0x62, 0x34, 0x68, 0x31, 0x76, 0x76, 0x73, 0x6B, 0x6F, 0x7A, 0x75, 0x71, 0x68, 0x33, 0x6A, 0x77
      };

      strcpy(device_ptr[i].device_id, buf);
      strcpy(device_ptr[i].xor_key, xor_key);
    }
    Serial.print(i);
    Serial.print(" => ");
    Serial.print(device_ptr[i].device_id);
    Serial.print(" : ");
    Serial.println(device_ptr[i].xor_key);
  }
}

void print_all_config(){
  
  uint32_t *save_ptr = device_config;

  save_ptr = ADDRESS_OF_PAGE(DEVICE_CONFIG_STORAGE);

  device *item = 0;

  Serial.println("-----------------------------");
  Serial.print("associated device list (size : ");
  Serial.print(config.device_num);
  Serial.println(") : ");

  for (int i = 0; i< config.device_num;i++){
    item = (device*)save_ptr;

    strcpy(device_ptr[i].device_id, item->device_id);
    strcpy(device_ptr[i].xor_key, item->xor_key);

    for (int j = 0 ; j < 8;j++){
      Serial.print (device_ptr[i].device_id[j]>>4, HEX) ; Serial.print (device_ptr[i].device_id[j]&15, HEX) ; Serial.print (" ") ;
    }
    Serial.print(" | ");
    for (int j = 0 ; j < 32;j++){
      Serial.print (device_ptr[i].xor_key[j]>>4, HEX) ; Serial.print (device_ptr[i].xor_key[j]&15, HEX) ; Serial.print (" ") ;
    }
    Serial.println("");
    save_ptr+=10;
  }
  Serial.println("-----------------------------");
}

void erase_host_config(){

  flashPageErase(PAGE_FROM_ADDRESS(device_config));

  for (int i = 0; i< MAX_ASSOCIATED_DEVICE;i++){
    strcpy(device_ptr[i].device_id, "");
    strcpy(device_ptr[i].xor_key, "");
  }
}

char* is_associated(char * device_id){

  for (int i = 0; i< MAX_ASSOCIATED_DEVICE;i++){

    if (memcmp(device_ptr[i].device_id,device_id,8)==0){
      return device_ptr[i].xor_key;
    }
  }
  return 0;
}

void write(){
  
  
  in_flash = (data_t*)ADDRESS_OF_PAGE(CONFIG_STORAGE);

  if (in_flash->flag != 0){

    Serial.println("writing default configuration");

    save_config(true);
    
    erase_host_config();
    //load_fake_host_config();
    /*
    for (int i = 0; i< MAX_ASSOCIATED_DEVICE;i++){
      write_host_config(&device_ptr[i]);
    }
    */

    RFduinoBLE.begin();

  }
  else {

    Serial.println("getting default configuration");

    for (byte i = 0; i < (4*N_BLOCK); i++){
      config.pass[i]=in_flash->pass[i];
    }
    
    config.flag=1;
    config.device_num=in_flash->device_num;
    config.lfsr=in_flash->lfsr;

    Serial.print("encrypted password : ");
    for (byte i = 0; i < (4*N_BLOCK); i++){
      Serial.print ( config.pass[i]>>4, HEX) ; Serial.print ( config.pass[i]&15, HEX) ; Serial.print (" ") ;
    }
    Serial.println();
    Serial.print("lfsr : ");
    Serial.println(config.lfsr);
  }

  print_all_config();
}

void setup() {

  Serial.begin(9600);
  
  pinMode(led, OUTPUT);

  pinMode(button, INPUT);
  
  digitalWrite(led, LOW);

  RFduinoBLE.advertisementData = "open LAB";

  RFduinoBLE.begin();

  //prekey(256,4);
  write();
}

bool interrupting(){

  if(RFduino_pinWoke(interupt_pin)){

    RFduino_resetPinWake(interupt_pin);

    return HIGH;
  }
  return LOW;
}

void loop() {
  
  RFduino_ULPDelay(INFINITE); 

  if(interrupting()){
    Serial.println("interrupting");
    save_config(false);
    add_device_pending=false;
    RFduinoBLE.begin();
  }
}

void interrupt(){

  Serial.println("interrupt()");

  NRF_GPIO->PIN_CNF[interupt_pin] = (GPIO_PIN_CNF_PULL_Pullup<<GPIO_PIN_CNF_PULL_Pos);

  RFduino_pinWake(interupt_pin,HIGH);

  NRF_GPIO->PIN_CNF[interupt_pin] = (GPIO_PIN_CNF_PULL_Pulldown<<GPIO_PIN_CNF_PULL_Pos);
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
  Serial.println("onDisconnect");
  use_local_keys=true;
  free(response);
  response=0;
  free(payload);
  payload=0;
  interrupt();
}

void sendCommandStatus(int cmd, bool status){

  char req[2]={0};
  uint8_t status_d = 1;
  if (status){
    status_d=0;
  }
  sprintf(req, "%c:%c", cmd,status_d);
  RFduinoBLE.send(req,10);
}

void sendCommandFailure(){
  char req[1]={0};
  sprintf(req, "%c", COMMAND_FAILURE);
  Serial.println("send association failure");
  RFduinoBLE.send(req,2);
}

void processEncryptedResponse(byte * check){

  Serial.println(COMMAND_STRING_ENUM[cmd]);

  if (strlen(token) == 0){
    switch(cmd){
      case COMMAND_ASSOCIATE_RESPONSE:
      {
        sendCommandStatus(COMMAND_ASSOCIATE,false);
        return;
      }
      case COMMAND_SET_PASSWORD_RESPONSE:
      {
        sendCommandStatus(COMMAND_SET_PASSWORD,false);
        return;
      }
      case COMMAND_SET_KEYS_RESPONSE:
      {
        sendCommandStatus(COMMAND_SET_KEY,false);
        return;
      }
    }
    return;
  }
}

void generate_key(byte * code,byte * key){
  
  uint16_t lfsr;

  lfsr = (code[0]<<8) + code[1];

  uint8_t j = 2;
  uint8_t k = 0;

  Serial.print("lfsr : ");
  Serial.println(lfsr);
  for (int i = 0; i  < 16;i++){

    if (i!=0 && ((i%4)==0)){

      Serial.print("test : ");
      Serial.print((int)code[j]);
      Serial.print( " et ");
      Serial.println((int)code[j+1]);

      lfsr = (code[j]<<8) + code[j+1];
      j+=2;
      Serial.print("lfsr : ");
      Serial.print(lfsr);
      Serial.print(" for index ");
      Serial.println(i);
    }
    uint16_t bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
    lfsr =  (lfsr >> 1) | (bit << 15);

    key[k] = (lfsr & 0xFF00)>>8;
    k++;
    key[k] = (lfsr & 0x00FF)>>0;
    k++;
    Serial.print("lfsr2 : ");
    Serial.print(lfsr);
  }
}

void generate_iv(byte * code, byte * iv){

  for (int i = 0; i  < 8;i++){
    iv[i] = code[i];
  }

  uint16_t lfsr;
  uint8_t k = 0;
  uint8_t j = 0;

  for (int i = 0; i  < 4;i++){
    lfsr = (code[j]<<8) + code[j+1];
    j+=2;
    uint16_t bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
    lfsr =  (lfsr >> 1) | (bit << 15);
    iv[k+8] = (lfsr & 0xFF00)>>8;
    k++;
    iv[k+8] = (lfsr & 0x00FF)>>0;
    k++;
  }
}

void generateRandomKeys(){

  //generate a key from 8 octet random
  uint16_t lfsr1 = random();
  uint16_t lfsr2 = random();
  uint16_t lfsr3 = random();
  uint16_t lfsr4 = random();

  char tokenStr[16];
  sprintf(tokenStr, "%04x%04x%04x%04x", lfsr1, lfsr2,lfsr3,lfsr4);
  Serial.println(tokenStr);

  byte code[8];
  code[0]=((lfsr1 & 0xFF00)>>8);
  code[1]=((lfsr1 & 0x00FF)>>0);
  code[2]=((lfsr2 & 0xFF00)>>8);
  code[3]=((lfsr2 & 0x00FF)>>0);
  code[4]=((lfsr3 & 0xFF00)>>8);
  code[5]=((lfsr3 & 0x00FF)>>0);
  code[6]=((lfsr4 & 0xFF00)>>8);
  code[7]=((lfsr4 & 0x00FF)>>0);

  generate_key(code,external_key);
  generate_iv(code,external_iv);

  Serial.println("code : ");
  for (int i = 0 ; i < 8;i++){
    Serial.print (code[i]>>4, HEX) ; Serial.print (code[i]&15, HEX) ; Serial.print (" ") ;
    if ((i!=0) && (i%15==0)){
      Serial.println("");
    }
  }
  Serial.println("");

  Serial.println("key2 : ");
  for (int i = 0 ; i < 32;i++){
    Serial.print (external_key[i]>>4, HEX) ; Serial.print (external_key[i]&15, HEX) ; Serial.print (" ") ;
    if ((i!=0) && (i%15==0)){
      Serial.println("");
    }
  }
  Serial.println("");

  Serial.println("iv2 : ");
  for (int i = 0 ; i < 16;i++){
    Serial.print (external_iv[i]>>4, HEX) ; Serial.print (external_iv[i]&15, HEX) ; Serial.print (" ") ;
    if ((i!=0) && (i%15==0)){
      Serial.println("");
    }
  }
  Serial.println("");
}

void processEncryptedFrame(byte * check){

  if (strlen(token) == 0){
    Serial.println("no token");
    sendCommandStatus(COMMAND_PUSH,false);
    return;
  }

  char device_id[8];
  Serial.print("device id : ");
  for (int i = 0; i< 8;i++){
    device_id[i] = check[i];
    Serial.print((uint8_t)device_id[i]);
    Serial.print(" ");
  }
  Serial.println("");

  char * xor_key = is_associated(device_id);

  if (xor_key!=0){

    Serial.println("device associated, continuing ...");

    char xored_hash[28];
    for (int i = 8;i < 36;i++){
      xored_hash[i-8] = check[i];
    }

    char xor_decoded[28];

    Serial.println("xor decoded : ");
    for (int j = 0 ;j  < 28;j++){
      xor_decoded[j]=xored_hash[j]^xor_key[j];
    }

    for (int i = 0; i < 8;i++){
      Serial.print((int)xor_decoded[i]);
      Serial.print("=>");
      Serial.print((int)token[i]);
      Serial.print(" ");
    }
    Serial.println("");

    if (memcmp(token,xor_decoded,8)==0){

      memset(token, 0, 8);

      char sent_pass[64];

      Serial.println("sent_pass :");
      for (int i = 8;i<28;i++){
        sent_pass[i-8] = xor_decoded[i];
        Serial.print(sent_pass[i-8]);
        Serial.print(" ");
      }
      for (int i = 20;i<64;i++){
        sent_pass[i]=0;
      }
      Serial.println("");

      //decrypt current pass
      byte pass[64];
      byte iv[16];
      byte cipher[64];
      byte succ = aes.set_key (config.key, 256);

      for (byte i = 0 ; i < 16 ; i++){
        iv[i] = config.iv[i] ;
      }
      for (byte i = 0 ; i< 64;i++){
        cipher[i] = config.pass[i];
      }
      //decrypt
      succ = aes.cbc_decrypt(cipher, pass, 4, iv) ;

      if (memcmp(sent_pass,pass,64)==0){

        switch (cmd){
          case COMMAND_PUSH:
          {
            Serial.println("push success");
            sendCommandStatus(COMMAND_PUSH,true);
            digitalWrite(led, HIGH);
            s1.attach(2);
            motor_process();
            s1.detach();
            digitalWrite(led, LOW);
            break;
          }
          case COMMAND_SET_PASSWORD:
          {
            generateRandomKeys();
            Serial.println("set password request success");
            use_local_keys= false;
            char buf[3]={0};
            sprintf(buf, "%c:%c", COMMAND_SET_PASSWORD, 2);
            RFduinoBLE.send(buf,3);
            break;
          }
          case COMMAND_SET_KEY:
          {
            generateRandomKeys();
            Serial.println("set keys request success");
            use_local_keys= false;
            char buf[3]={0};
            sprintf(buf, "%c:%c", COMMAND_SET_KEY, 2);
            RFduinoBLE.send(buf,3);
            break;
          }
        }
      }
      else{
        Serial.println("cmd failure : bad pass");
        sendCommandStatus(cmd,false);
      } 
    }
    else{
      memset(token, 0, 8);
      Serial.println("cmd failure : bad token");
      sendCommandStatus(cmd,false);
    }
  }
  else{
    Serial.println("device not associated, aborting ...");
    sendCommandStatus(cmd,false);
  }
}

void executeCmd(byte *check){

  for (byte ph = 0; ph < (4*16); ph++){
    Serial.print (check[ph]>>4, HEX) ; Serial.print (check[ph]&15, HEX) ; Serial.print (" ") ;
    if ((ph!=0) && (ph%15==0)){
      Serial.println("");
    }
  }

  Serial.println("");
  Serial.print(COMMAND_STRING_ENUM[cmd]);
  Serial.println(" processing ...");

  switch(cmd){

    case COMMAND_ASSOCIATION_STATUS:
    {
      if (strlen(token) == 0){
        sendCommandStatus(COMMAND_ASSOCIATION_STATUS,false);
        return;
      }

      char device_id[8];
      Serial.print("device id : ");
      for (int i = 0; i< 8;i++){
        device_id[i] = check[i];
        Serial.print((uint8_t)device_id[i]);
        Serial.print(" ");
      }
      Serial.println("");

      char * xor_key = is_associated(device_id);

      if (xor_key!=0){
        Serial.println("device associated, continuing ...");
        char xored_token[8];
        for (int i = 8;i < 16;i++){
          xored_token[i-8] = check[i];
        }

        char xor_decoded[8];

        Serial.println("xor decoded : ");
        for (int j = 0 ;j  < 8;j++){
          xor_decoded[j]=xored_token[j]^xor_key[j];
          Serial.print((int)xor_decoded[j]);
          Serial.print("=>");
          Serial.print((int)token[j]);
          Serial.print(" ");
        }
        Serial.println("");

        if (memcmp(token,xor_decoded,8)==0){
          memset(token, 0, 8);
          Serial.println("association status successfully requested");
          sendCommandStatus(COMMAND_ASSOCIATION_STATUS,true);
        }
        else{
          memset(token, 0, 8);
          Serial.println("association status failure : not associated");
          sendCommandStatus(COMMAND_ASSOCIATION_STATUS,false);
        }
      }
      else{
        Serial.println("device not associated, aborting ...");
        sendCommandStatus(COMMAND_ASSOCIATION_STATUS,false);
      }
      break;
    }
    case COMMAND_PUSH:
    {
      Serial.println("processing push...");
      processEncryptedFrame(check);
      break;
    }
    case COMMAND_SET_PASSWORD:
    {
      Serial.println("processing set password...");
      processEncryptedFrame(check);
      break;
    }
    case COMMAND_SET_KEY:
    {
      Serial.println("processing set keys...");
      processEncryptedFrame(check);
      break;
    }
    case COMMAND_SET_PASSWORD_RESPONSE:
    {
      if (strlen(token) == 0){
        Serial.println("no token");
        sendCommandStatus(COMMAND_PUSH,false);
        return;
      }

      char device_id[8];
      Serial.print("device id : ");
      for (int i = 0; i< 8;i++){
        device_id[i] = check[i];
        Serial.print((uint8_t)device_id[i]);
        Serial.print(" ");
      }
      Serial.println("");

      char * xor_key = is_associated(device_id);

      if (xor_key!=0){

        Serial.println("device associated, continuing ...");
        
        char xored_hash[56];
        for (int i = 8;i < 64;i++){
          xored_hash[i-8] = check[i];
        }

        char xor_decoded[28];
        Serial.println("xor decoded : ");
        for (int j = 0 ;j  < 28;j++){
          xor_decoded[j]=xored_hash[j]^xor_key[j];
        }

        for (int i = 0; i < 8;i++){
          Serial.print((int)xor_decoded[i]);
          Serial.print("=>");
          Serial.print((int)token[i]);
          Serial.print(" ");
        }
        Serial.println("");

        if (memcmp(token,xor_decoded,8)==0){

          memset(token, 0, 8);

          byte succ = aes.set_key (config.key, 256);

          int blocks = 4;
          byte cipher [4*N_BLOCK] ;
          byte input[64];
          byte iv [N_BLOCK];

          for (int i = 0; i < 20;i++){
            input[i]=xor_decoded[i+8];
          }
          for (int i = 20; i < 64;i++){
            input[i]=0;
          }

          if (blocks == 1){
            succ = aes.encrypt(input, cipher) ;
          }
          else {

            for (byte i = 0 ; i < N_BLOCK ; i++){
              iv[i] = config.iv[i] ;
            }
            succ = aes.cbc_encrypt(input, cipher, blocks, iv) ;
          }


          for (byte i = 0; i < (4*N_BLOCK); i++){
            config.pass[i]=cipher[i];
          }

          for (byte i = 0; i < (4*N_BLOCK); i++){
            Serial.print (cipher[i]>>4, HEX) ; Serial.print (cipher[i]&15, HEX) ; Serial.print (" ") ;
          }
          Serial.println();

          for (int i =0;i<64;i++){
            config.pass[i]=cipher[i];
          }

          Serial.println("set password success");
          sendCommandStatus(cmd,true);
        }
        else{
          memset(token, 0, 8);
          Serial.println("cmd failure : bad token");
          sendCommandStatus(cmd,false);
        }
      }
      else{
        Serial.println("device not associated, aborting ...");
        sendCommandStatus(cmd,false);
      }
      break;
    }
    case COMMAND_SET_KEYS_RESPONSE:
    {
      Serial.println("processing set keys");

      if (strlen(token) == 0){
        Serial.println("no token");
        sendCommandStatus(COMMAND_PUSH,false);
        return;
      }

      char device_id[8];
      Serial.print("device id : ");
      for (int i = 0; i< 8;i++){
        device_id[i] = check[i];
        Serial.print((uint8_t)device_id[i]);
        Serial.print(" ");
      }
      Serial.println("");

      char * xor_key = is_associated(device_id);

      if (xor_key!=0){

        Serial.println("device associated, continuing ...");
        
        char xored_hash[64];
        for (int i = 8;i < 64;i++){
          xored_hash[i-8] = check[i];
        }

        char xor_decoded[64];

        Serial.println("xor decoded : ");
        for (int j = 0 ;j  < 32;j++){
          xor_decoded[j]=xored_hash[j]^xor_key[j];
        }
        for (int j = 32 ;j  < 64;j++){
          xor_decoded[j]=xored_hash[j]^xor_key[j-32];
        }

        for (int i = 0; i < 8;i++){
          Serial.print((int)xor_decoded[i]);
          Serial.print("=>");
          Serial.print((int)token[i]);
          Serial.print(" ");
        }
        Serial.println("");

        if (memcmp(token,xor_decoded,8)==0){

          //decrypt current pass
          byte pass[64];
          byte iv[16];
          byte cipher[64];
          byte succ = aes.set_key (config.key, 256);

          for (byte i = 0 ; i < 16 ; i++){
            iv[i] = config.iv[i] ;
          }
          for (byte i = 0 ; i< 64;i++){
            cipher[i] = config.pass[i];
          }
          //decrypt
          succ = aes.cbc_decrypt(cipher, pass, 4, iv) ;

          memset(token, 0, 8);
          Serial.println("SET KEYS OK");

          for (int i = 0; i < 32;i++){
            config.key[i]=xor_decoded[i+8];
          }

          for (int i = 0; i < 16;i++){
            config.iv[i]=xor_decoded[i+8+32];
          }

          succ = aes.set_key (config.key, 256);

          for (byte i = 0 ; i < N_BLOCK ; i++){
            iv[i] = config.iv[i] ;
          }
          succ = aes.cbc_encrypt(pass, cipher, 4, iv) ;

          for (byte i = 0; i < (4*N_BLOCK); i++){
            config.pass[i]=cipher[i];
          }

          Serial.println("set key success");
          Serial.println(cmd);
          sendCommandStatus(cmd,true);
        }
        else{
          memset(token, 0, 8);
          Serial.println("cmd failure : bad token");
          sendCommandStatus(cmd,false);
        }
      }
      else{
        Serial.println("device not associated, aborting ...");
        sendCommandStatus(cmd,false);
      }

      break;
    }
    case COMMAND_ASSOCIATE_RESPONSE:
    {
      Serial.println("processing associate response");

      if (strlen(token) == 0){
        sendCommandStatus(COMMAND_ASSOCIATE,false);
        return;
      }

      char device_id[8];
      Serial.print("device id : ");
      for (int i = 0; i< 8;i++){
        device_id[i] = check[i];
        Serial.print((uint8_t)device_id[i]);
        Serial.print(" ");
      }
      Serial.println("");

      char xor_key[32];
      Serial.print("xor key : ");
      for (int i = 0; i< 32;i++){
        xor_key[i] = check[i+8];
        Serial.print((uint8_t)xor_key[i]);
        Serial.print(" ");
      }
      Serial.println("");

      char token_field[8];
      Serial.print("token : ");
      for (int i = 0; i< 8;i++){
        token_field[i] = check[i+8+32];
        Serial.print((uint8_t)token_field[i]);
        Serial.print(" ");
      }
      Serial.println("");

      if (memcmp(token,token_field,8)==0){
        
        memset(token, 0, 8);
        Serial.println("association success");
        //record xor / device id
        add_device(device_id,xor_key);
        
        byte key_data[64];
        byte iv_data[64];

        for (int i = 0; i < 32;i++){
          key_data[i] = config.key[i];
        }
        for (int i = 32 ; i < 64;i++){
          key_data[i]= 0;
        }
        for (int i = 0; i < 16;i++){
          iv_data[i] = config.iv[i];
        }
        for (int i = 16; i < 64;i++){
          iv_data[i] = 0;
        }

        Serial.println("CURRENT KEY");
        for (byte i = 0; i < 32; i++){
          Serial.print (key_data[i]>>4, HEX) ; Serial.print (key_data[i]&15, HEX) ; Serial.print (" ") ;
        }
        Serial.println();
        Serial.println("CURRENT IV");
        for (byte i = 0; i < 16; i++){
          Serial.print (iv_data[i]>>4, HEX) ; Serial.print (iv_data[i]&15, HEX) ; Serial.print (" ") ;
        }

        Serial.println("EXTERNAL KEY");
        for (byte i = 0; i < 32; i++){
          Serial.print (external_key[i]>>4, HEX) ; Serial.print (external_key[i]&15, HEX) ; Serial.print (" ") ;
        }
        Serial.println();
        Serial.println("EXTERNAL IV");
        for (byte i = 0; i < 16; i++){
          Serial.print (external_iv[i]>>4, HEX) ; Serial.print (external_iv[i]&15, HEX) ; Serial.print (" ") ;
        }
        Serial.println();

        byte succ = aes.set_key (external_key, 256);

        int blocks = 4;
        byte cipher [64] ;
        byte iv [16];
        for (byte i = 0 ; i < 16 ; i++){
          iv[i] = external_iv[i] ;
        }
        succ = aes.cbc_encrypt(key_data, cipher, blocks, iv) ;

        byte check[64];
        for (byte i = 0 ; i < 16 ; i++){
          iv[i] = external_iv[i] ;
        }
        //decrypt
        succ = aes.cbc_decrypt(cipher, check, 4, iv) ;
        Serial.println("DECRYPT KEY");
        for (byte i = 0; i < 64; i++){
          Serial.print (check[i]>>4, HEX) ; Serial.print (check[i]&15, HEX) ; Serial.print (" ") ;
        }
        Serial.println();

        state = STATE_SENDING;
        data_length = 128;
        response = (uint8_t*)malloc(data_length + 1);
        memset(response, 0, data_length + 1);

        for (byte i = 0; i < 64; i++){
          Serial.print (cipher[i]>>4, HEX) ; Serial.print (cipher[i]&15, HEX) ; Serial.print (" ") ;
          response[i]=cipher[i];
        }
        Serial.println();

        for (byte i = 0 ; i < N_BLOCK ; i++){
          iv[i] = external_iv[i] ;
        }
        succ = aes.cbc_encrypt(iv_data, cipher, blocks, iv) ;

        for (byte i = 0 ; i < 16 ; i++){
          iv[i] = external_iv[i] ;
        }
        //decrypt
        succ = aes.cbc_decrypt(cipher, check, 4, iv) ;
        Serial.println("DECRYPT IV");
        for (byte i = 0; i < 64; i++){
          Serial.print (check[i]>>4, HEX) ; Serial.print (check[i]&15, HEX) ; Serial.print (" ") ;
        }
        Serial.println();


         for (byte i = 0; i < 64; i++){
          Serial.print (cipher[i]>>4, HEX) ; Serial.print (cipher[i]&15, HEX) ; Serial.print (" ") ;
          response[i+64]=cipher[i];
        }
        Serial.println();

        sending_index=0;
        
        Serial.println(data_length);

        char req[10]={0};
        sprintf(req, "%c:%c:%d", COMMAND_ASSOCIATE_RESPONSE, STATE_SENDING,data_length);
        Serial.print("sending : ");
        Serial.println(req);
        RFduinoBLE.send(req,10);

      }
      else{
        memset(token, 0, 8);
        Serial.println("association failure : not associated");
        sendCommandStatus(COMMAND_ASSOCIATE,false);
      }

      break;
    }
  }
}

void processHeaderFrame(int cmd,char *data, int len){

  Serial.println(COMMAND_STRING_ENUM[cmd]);

  if (len>1){
    state = STATE_PROCESSING;
    data_length = data[2] + (data[1]<<8);
    payload = (uint8_t*)malloc(data_length + 1);
    memset(payload, 0, data_length + 1);

    Serial.print("cmd: ");
    Serial.println(cmd);
    Serial.print("data_length: ");
    Serial.println(data_length);
  }
  else{
    Serial.print("error length error for ");
    Serial.println(COMMAND_STRING_ENUM[cmd]);
  }
}

void RFduinoBLE_onReceive(char *data, int len){

  switch (state){

    case STATE_NONE:
    {
      free(payload);
      payload = 0;
      offset=0;
    
      cmd = data[0];

      Serial.print("receive command ");
      Serial.print(cmd);
      Serial.print(" ");

      uint8_t ret = 0;

      switch (cmd){

        case COMMAND_GET_TOKEN:
        {
          Serial.println(COMMAND_STRING_ENUM[cmd]);

          uint16_t lfsr1 = random();
          uint16_t lfsr2 = random();
          uint16_t lfsr3 = random();
          uint16_t lfsr4 = random();

          char tokenStr[16];
          sprintf(tokenStr, "%04x%04x%04x%04x", lfsr1, lfsr2,lfsr3,lfsr4);
          char buf[50]={0};
          sprintf(buf, "%c:%s", COMMAND_GET_TOKEN, tokenStr);

          token[0]=((lfsr1 & 0xFF00)>>8);
          token[1]=((lfsr1 & 0x00FF)>>0);
          token[2]=((lfsr2 & 0xFF00)>>8);
          token[3]=((lfsr2 & 0x00FF)>>0);
          token[4]=((lfsr3 & 0xFF00)>>8);
          token[5]=((lfsr3 & 0x00FF)>>0);
          token[6]=((lfsr4 & 0xFF00)>>8);
          token[7]=((lfsr4 & 0x00FF)>>0);

          Serial.print("send token : ");
          Serial.println(tokenStr);
          ret=1;
          RFduinoBLE.send(buf,20);
          break;
        }
        case COMMAND_ASSOCIATION_STATUS:
        {
          processHeaderFrame(cmd,data,len);
          break;
        }
        case COMMAND_PUSH:
        {
          processHeaderFrame(cmd,data,len);
          break;
        }
        case COMMAND_ASSOCIATE_RESPONSE:
        {
          processHeaderFrame(cmd,data,len);
          break;
        }
        case COMMAND_SET_PASSWORD_RESPONSE:
        {
          processHeaderFrame(cmd,data,len);
          break;
        }
        case COMMAND_SET_KEYS_RESPONSE:
        {
          processHeaderFrame(cmd,data,len);
          break;
        }
        case COMMAND_SET_PASSWORD:
        {
          processHeaderFrame(cmd,data,len);
          break;
        }
        case COMMAND_SET_KEY:
        {
          processHeaderFrame(cmd,data,len);
          break;
        }
        case COMMAND_ASSOCIATE:
        {
          Serial.println(COMMAND_STRING_ENUM[cmd]);

          generateRandomKeys();

          use_local_keys= false;

          char buf[3]={0};
          sprintf(buf, "%c:%c", COMMAND_ASSOCIATE, 2);
          RFduinoBLE.send(buf,3);

          break;
        }
        case COMMAND_RECEIVE_KEYS:
        {
          Serial.println("sending keys");
          char buf[18]={0};
          int diff = data_length-sending_index;
          Serial.println(diff);

          if (diff<=18){
            Serial.println("sending last frame");

            for (int i = 0;i < diff;i++){
              buf[i]=response[i+sending_index];
            }
            Serial.println(data_length-sending_index);
            RFduinoBLE.send(buf,(data_length-sending_index));
            sending_index=0;
            free(response);
            response=0;
          }
          else{
            for (int i = 0;i < 18;i++){
              buf[i]=response[i+sending_index];
            }
            sending_index+=18;
            RFduinoBLE.send(buf,18);
          }
          break;
        }
      }
      break;
    }
    case STATE_PROCESSING:
    {
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

        if (use_local_keys){

          for (byte i = 0 ; i < 16 ; i++){
            iv[i] = config.iv[i] ;
          }
          succ = aes.set_key (config.key, 256);
        }
        else{
          
          Serial.println("decrypting with external keys");

          Serial.println("key2 : ");
          for (int i = 0 ; i < 32;i++){
            Serial.print (external_key[i]>>4, HEX) ; Serial.print (external_key[i]&15, HEX) ; Serial.print (" ") ;
            if ((i!=0) && (i%15==0)){
              Serial.println("");
            }
          }
          Serial.println("");

          Serial.println("iv2 : ");
          for (int i = 0 ; i < 16;i++){
            Serial.print (external_iv[i]>>4, HEX) ; Serial.print (external_iv[i]&15, HEX) ; Serial.print (" ") ;
            if ((i!=0) && (i%15==0)){
              Serial.println("");
            }
          }
          Serial.println("");

          succ = aes.set_key (external_key, 256);

          for (byte i = 0 ; i < 16 ; i++){
            iv[i] = external_iv[i] ;
          }
          use_local_keys=true;
        }

        //decrypt
        succ = aes.cbc_decrypt(cipher, check, 4, iv) ;

        if (succ==0){
          executeCmd(check);
        }
        else{
          //send failure
          sendCommandFailure();
        }
        free(payload);
        payload = 0;
        state=STATE_NONE;
      }
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