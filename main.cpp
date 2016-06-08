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

#if 0
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
#endif 

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
};

data_t config = { 
  "admin" ,
  0
};

Servo s1;

int led = 3;
int button = 5;

data_t *in_flash = (data_t*)ADDRESS_OF_PAGE(CONFIG_STORAGE);

uint32_t *device_config = ADDRESS_OF_PAGE(DEVICE_CONFIG_STORAGE);

void save_config(bool default_config){

    if (default_config){

      int blocks = 4;
      byte cipher [4*N_BLOCK] ;
      byte succ = 0;
      byte iv [N_BLOCK];

      if (blocks == 1){
        succ = aes.encrypt(default_pass, cipher) ;
      }
      else {

        for (byte i = 0 ; i < N_BLOCK ; i++){
          iv[i] = my_iv[i] ;
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
    }
    
    while (!RFduinoBLE.radioActive);
    while (RFduinoBLE.radioActive);

    RFduinoBLE.end();

    flashPageErase(PAGE_FROM_ADDRESS(in_flash));

    int rc = flashWriteBlock(in_flash, &config, sizeof(config));

}

void load_fake_host_config(){

  for (int i = 0; i< MAX_ASSOCIATED_DEVICE;i++){

    if (i>0){
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

  for (int i = 0; i< MAX_ASSOCIATED_DEVICE;i++){
    item = (device*)save_ptr;

    strcpy(device_ptr[i].device_id, item->device_id);
    strcpy(device_ptr[i].xor_key, item->xor_key);

    Serial.print(device_ptr[i].device_id);
    Serial.print(" : ");
    Serial.println(device_ptr[i].xor_key);
    save_ptr+=10;
  }
}

void erase_host_config(){

  flashPageErase(PAGE_FROM_ADDRESS(device_config));

  for (int i = 0; i< MAX_ASSOCIATED_DEVICE;i++){
    strcpy(device_ptr[i].device_id, "");
    strcpy(device_ptr[i].xor_key, "");
  }
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

  if (in_flash->flag != 1){

    Serial.println("writing default configuration");

    save_config(true);
    
    erase_host_config();
    load_fake_host_config();
    
    for (int i = 0; i< MAX_ASSOCIATED_DEVICE;i++){
      write_host_config(&device_ptr[i]);
    }

    RFduinoBLE.begin();

  }
  else {

    Serial.println("getting default configuration");

    for (byte i = 0; i < (4*N_BLOCK); i++){
      config.pass[i]=in_flash->pass[i];
    }
    
    config.flag=1;

    config.lfsr=in_flash->lfsr;

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

  //prekey(256,4);

  byte succ = aes.set_key (key, 256);
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
    save_config(false);
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
  interrupt();
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

void sendAssociationStatus(bool status){
  
  char req[2]={0};
  uint8_t status_d = 1;
  if (status){
    status_d=0;
  }
  sprintf(req, "%d:%d", COMMAND_ASSOCIATION_STATUS,status_d);
  RFduinoBLE.send(req,3);
}

void sendCommandFailure(){
  char req[1]={0};
  sprintf(req, "%d", COMMAND_FAILURE);
  Serial.println("send association failure");
  RFduinoBLE.send(req,2);
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
        sendAssociationStatus(false);
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
          sendAssociationStatus(true);
        }
        else{
          memset(token, 0, 8);
          Serial.println("association status failure : not associated");
          sendAssociationStatus(false);
        }
      }
      else{
        Serial.println("device not associated, aborting ...");
        sendAssociationStatus(false);
      }
      break;
    }
    case COMMAND_PUSH:
    {
      break;
    }
    case COMMAND_SET_PASSWORD:
    {
      break;
    }
    case COMMAND_ASSOCIATE:
    {
      break;
    }
    case COMMAND_SET_KEY:
    {
      break;
    }
  }
}

uint16_t bit;

uint16_t random()
{
  bit  = ((config.lfsr >> 0) ^ (config.lfsr >> 2) ^ (config.lfsr >> 3) ^ (config.lfsr >> 5) ) & 1;
  return config.lfsr =  (config.lfsr >> 1) | (bit << 15);
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
        case COMMAND_GET_TOKEN:{
          Serial.println(COMMAND_STRING_ENUM[cmd]);

          uint16_t lfsr1 = random();
          uint16_t lfsr2 = random();
          uint16_t lfsr3 = random();
          uint16_t lfsr4 = random();

          char tokenStr[16];
          sprintf(tokenStr, "%04x%04x%04x%04x", lfsr1, lfsr2,lfsr3,lfsr4);
          char buf[50]={0};
          sprintf(buf, "%d:%s", COMMAND_GET_TOKEN, tokenStr);

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
          Serial.println(COMMAND_STRING_ENUM[cmd]);
          state = STATE_PROCESSING;
          data_length = data[2] + (data[1]<<8);
          payload = (uint8_t*)malloc(data_length + 1);
          memset(payload, 0, data_length + 1);

          Serial.print("cmd: ");
          Serial.println(cmd);
          Serial.print("data_length: ");
          Serial.println(data_length);
          break;
        }
        case COMMAND_PUSH:
        {
          Serial.println(COMMAND_STRING_ENUM[cmd]);
          break;
        }
        case COMMAND_SET_PASSWORD:
        {
          Serial.println(COMMAND_STRING_ENUM[cmd]);
          break;
        }
        case COMMAND_ASSOCIATE:
        {
          Serial.println(COMMAND_STRING_ENUM[cmd]);
          break;
        }
        case COMMAND_SET_KEY:
        {
          Serial.println(COMMAND_STRING_ENUM[cmd]);
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
        for (byte i = 0 ; i < 16 ; i++){
          iv[i] = my_iv[i] ;
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