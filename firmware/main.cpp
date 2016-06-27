#include "Arduino.h"
#include <Servo.h>
#include <RFduinoBLE.h>
#include <string>
#include <lcd/LiquidCrystal.h>
#include <aes/AES.h>
#include "config/config.h"
#include "constants.h"
#include "crypto/crypto.h"
#include "model.h"


Crypto crypto;
Config config;

char token[16];
uint8_t state = STATE_NONE;
uint8_t state_count= 0;
uint8_t cmd = 0;
uint16_t data_length = 0;
bool use_local_keys = true;
uint16_t sending_index = 0;
bool code_expiring=false;
unsigned long code_expiring_time=0;
bool save_conf = false;

AES aes;

byte external_key[32];
byte external_iv[16];

uint8_t* payload;
uint8_t* response;
uint8_t offset = 0;

Servo s1;

LiquidCrystal lcd(6, 5, 4, 3, 1, 0);

void print_lcd_message(char * header,char * message){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(header);
  lcd.setCursor(0,1);
  lcd.print(message);
}

void print_default_message(){
  //print_lcd_message(config.get_config().top_message,config.get_config().bottom_message);
}

void generateRandomKeys(){

  char token_str[16];
  byte code[8];
  crypto.generateRandomKeys(token_str,code);
  crypto.generate_key(code,external_key);
  crypto.generate_iv(code,external_iv);
  code_expiring_time = millis()+CODE_EXPIRING_TIME_SECONDS*1000;
  code_expiring=true;

  print_lcd_message("pairing code :",token_str);
}

void setup() {

  #ifdef __PRINT_LOG__
   Serial.begin(9600);
  #endif //__PRINT_LOG__
  
  lcd.begin(16, 2);

  config.init(&lcd,&aes);

  RFduinoBLE.advertisementData = ADVERTISMENT_DATA;

  RFduinoBLE.begin();

  config.restore();

  print_default_message();
}

bool interrupting(){

  if(RFduino_pinWoke(INTERRUPT_PIN)){

    RFduino_resetPinWake(INTERRUPT_PIN);

    return HIGH;
  }
  return LOW;
}

void loop() {
 
  //RFduinoBLE_ULPDelay(INFINITE); 
  delay(500);
  
  if(interrupting()){

    #ifdef __PRINT_LOG__
      Serial.println("interrupting");
    #endif //__PRINT_LOG__

    if (save_conf || config.is_device_pending()){
      config.save_config(false);
      RFduinoBLE.begin();
    }
    
    save_conf=false;
    config.set_device_pending(false);
    print_default_message();
  }
  
  if (code_expiring){

    int rem = code_expiring_time-millis();

    if (code_expiring_time!=0 && rem<=0){
      Serial.println("code has expired");
      code_expiring_time=0;
      code_expiring=false;
      memset(token, 0, 8);
      memset(external_iv, 0, 16);
      memset(external_key, 0, 32);
      print_default_message();
    }
  }
}

void interrupt(){

  #ifdef __PRINT_LOG__
    Serial.println("interrupt()");
  #endif //__PRINT_LOG__

  NRF_GPIO->PIN_CNF[INTERRUPT_PIN] = (GPIO_PIN_CNF_PULL_Pullup<<GPIO_PIN_CNF_PULL_Pos);

  RFduino_pinWake(INTERRUPT_PIN,HIGH);

  NRF_GPIO->PIN_CNF[INTERRUPT_PIN] = (GPIO_PIN_CNF_PULL_Pulldown<<GPIO_PIN_CNF_PULL_Pos);
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

  #ifdef __PRINT_LOG__
    Serial.println("onDisconnect");
  #endif //__PRINT_LOG__
  memset(token, 0, 8);
  memset(external_iv, 0, 16);
  memset(external_key, 0, 32);
  code_expiring=false;
  code_expiring_time=0;
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

  #ifdef __PRINT_LOG__
    Serial.println("send association failure");
  #endif //__PRINT_LOG__

  RFduinoBLE.send(req,2);
}

void processEncryptedResponse(byte * check){

  #ifdef __PRINT_LOG__
    Serial.println(COMMAND_STRING_ENUM[cmd]);
  #endif //__PRINT_LOG__

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

void processEncryptedFrame(byte * check){

  if (strlen(token) == 0){

    #ifdef __PRINT_LOG__
      Serial.println("no token");
    #endif //__PRINT_LOG__

    print_lcd_message("command failure:","no token sent");

    sendCommandStatus(COMMAND_PUSH,false);
    return;
  }

  char device_id[8];

  for (int i = 0; i< 8;i++){
    device_id[i] = check[i];
  }

  char * xor_key = config.is_associated(device_id);

  if (xor_key!=0){

    #ifdef __PRINT_LOG__
      Serial.println("device associated, continuing ...");
    #endif //__PRINT_LOG__

    char xored_hash[28];
    for (int i = 8;i < 36;i++){
      xored_hash[i-8] = check[i];
    }

    char xor_decoded[28];

    
    for (int j = 0 ;j  < 28;j++){
      xor_decoded[j]=xored_hash[j]^xor_key[j];
    }

    #ifdef __PRINT_LOG__
      Serial.println("xor decoded : ");
      for (int i = 0; i < 8;i++){
        Serial.print((int)xor_decoded[i]);
        Serial.print("=>");
        Serial.print((int)token[i]);
        Serial.print(" ");
      }
      Serial.println("");
    #endif //__PRINT_LOG__

    if (memcmp(token,xor_decoded,8)==0){

      memset(token, 0, 8);

      char sent_pass[64];
      char pass_temp[64];

      for (int i = 8;i<28;i++){
        sent_pass[i-8] = xor_decoded[i];
      }
      for (int i = 20;i<64;i++){
        sent_pass[i]=0;
      }

      //decrypt current pass
      byte pass[64];

      byte iv[16];
      byte cipher[64];
      byte key[32];
      for (int i = 0; i < 32;i++){
        key[i]=config.get_config().key[i];
      }
      byte succ = aes.set_key (key, 256);

      for (byte i = 0 ; i < 16 ; i++){
        iv[i] = config.get_config().iv[i] ;
      }
      for (byte i = 0 ; i< 64;i++){
        cipher[i] = config.get_config().pass[i];
      }
      //decrypt
      succ = aes.cbc_decrypt(cipher, pass, 4, iv) ;
      
      for (int i  =0; i  < 64;i++){
        pass_temp[i]=pass[i];
      }

      if (memcmp(sent_pass,pass_temp,64)==0){

        switch (cmd){
          case COMMAND_PUSH:
          {
            #ifdef __PRINT_LOG__
              Serial.println("push success");
            #endif //__PRINT_LOG__

            print_lcd_message("command status :","push success");

            sendCommandStatus(COMMAND_PUSH,true);
            s1.attach(2);
            motor_process();
            s1.detach();
            break;
          }
          case COMMAND_MESSAGE_TOP:
          {
            #ifdef __PRINT_LOG__
              Serial.println("set message top success");
            #endif //__PRINT_LOG__

            char top_message[28];
            for (int i = 36;i < 64;i++){
              top_message[i-36] = check[i];
            }
            config.set_top_message(top_message);

            save_conf = true;

            sendCommandStatus(cmd,true);
            break;
          }
          case COMMAND_MESSAGE_BOTTOM:
          {
            #ifdef __PRINT_LOG__
              Serial.println("set message bottom success");
            #endif //__PRINT_LOG__

            char bottom_message[28];
            for (int i = 36;i < 64;i++){
              bottom_message[i-36] = check[i];
            }
            config.set_bottom_message(bottom_message);

            save_conf = true;

            sendCommandStatus(cmd,true);
            break;
          }
          case COMMAND_SET_PASSWORD:
          {
            generateRandomKeys();

            #ifdef __PRINT_LOG__
              Serial.println("set password request success");
            #endif //__PRINT_LOG__

            use_local_keys= false;
            char buf[3]={0};
            sprintf(buf, "%c:%c", COMMAND_SET_PASSWORD, 2);
            RFduinoBLE.send(buf,3);
            break;
          }
          case COMMAND_SET_KEY:
          {
            generateRandomKeys();

            #ifdef __PRINT_LOG__
              Serial.println("set keys request success");
            #endif //__PRINT_LOG__

            use_local_keys= false;
            char buf[3]={0};
            sprintf(buf, "%c:%c", COMMAND_SET_KEY, 2);
            RFduinoBLE.send(buf,3);
            break;
          }
          case COMMAND_DISASSOCIATION:
          {
            #ifdef __PRINT_LOG__
              Serial.println("disassociation success");
            #endif //__PRINT_LOG__

            print_lcd_message("command status :","unpairing OK");

            config.remove_device(device_id);
            char buf[3]={0};
            sprintf(buf, "%c:%c", COMMAND_DISASSOCIATION, 0);
            RFduinoBLE.send(buf,3);
          }
        }
      }
      else{
        #ifdef __PRINT_LOG__
          Serial.println("cmd failure : bad pass");
        #endif //__PRINT_LOG__

        sendCommandStatus(cmd,false);
      } 
    }
    else{
      print_lcd_message("command failure:","bad token");

      memset(token, 0, 8);

      #ifdef __PRINT_LOG__
        Serial.println("cmd failure : bad token");
      #endif //__PRINT_LOG__

      sendCommandStatus(cmd,false);
    }
  }
  else{
    print_lcd_message("command failure:","not associated");

    #ifdef __PRINT_LOG__
      Serial.println("device not associated, aborting ...");
    #endif //__PRINT_LOG__

    sendCommandStatus(cmd,false);
  }
}

void executeCmd(byte *check){

  #ifdef __PRINT_LOG__
    for (byte ph = 0; ph < (4*16); ph++){
      Serial.print (check[ph]>>4, HEX) ; Serial.print (check[ph]&15, HEX) ; Serial.print (" ") ;
      if ((ph!=0) && (ph%15==0)){
        Serial.println("");
      }
    }

    Serial.println("");
    Serial.print(COMMAND_STRING_ENUM[cmd]);
    Serial.println(" processing ...");
  #endif //__PRINT_LOG__

  switch(cmd){

    case COMMAND_ASSOCIATION_STATUS:
    {
      if (strlen(token) == 0){
        sendCommandStatus(COMMAND_ASSOCIATION_STATUS,false);
        return;
      }

      char device_id[8];

      for (int i = 0; i< 8;i++){
        device_id[i] = check[i];
      }

      char * xor_key = config.is_associated(device_id);

      if (xor_key!=0){

        #ifdef __PRINT_LOG__
          Serial.println("device associated, continuing ...");
        #endif //__PRINT_LOG__

        char xored_token[8];
        for (int i = 8;i < 16;i++){
          xored_token[i-8] = check[i];
        }

        char xor_decoded[8];

        for (int j = 0 ;j  < 8;j++){
          xor_decoded[j]=xored_token[j]^xor_key[j];
        }

        if (memcmp(token,xor_decoded,8)==0){
          memset(token, 0, 8);

          #ifdef __PRINT_LOG__
            Serial.println("association status successfully requested");
          #endif //__PRINT_LOG__
          sendCommandStatus(COMMAND_ASSOCIATION_STATUS,true);
        }
        else{
          memset(token, 0, 8);

          #ifdef __PRINT_LOG__
            Serial.println("association status failure : not associated");
          #endif //__PRINT_LOG__
          sendCommandStatus(COMMAND_ASSOCIATION_STATUS,false);
        }
      }
      else{
        #ifdef __PRINT_LOG__
          Serial.println("device not associated, aborting ...");
        #endif //__PRINT_LOG__
        sendCommandStatus(COMMAND_ASSOCIATION_STATUS,false);
      }
      break;
    }
    case COMMAND_PUSH:
    {
      #ifdef __PRINT_LOG__
        Serial.println("processing push...");
      #endif //__PRINT_LOG__
      processEncryptedFrame(check);
      break;
    }
    case COMMAND_MESSAGE_BOTTOM:
    case COMMAND_MESSAGE_TOP:
    {
      #ifdef __PRINT_LOG__
        Serial.println("processing message...");
      #endif //__PRINT_LOG__
      processEncryptedFrame(check);
      break;
    }
    case COMMAND_SET_PASSWORD:
    {
      #ifdef __PRINT_LOG__
        Serial.println("processing set password...");
      #endif //__PRINT_LOG__
      processEncryptedFrame(check);
      break;
    }
    case COMMAND_SET_KEY:
    {
      #ifdef __PRINT_LOG__
        Serial.println("processing set keys...");
      #endif //__PRINT_LOG__
      processEncryptedFrame(check);
      break;
    }
    case COMMAND_DISASSOCIATION:
    {
      #ifdef __PRINT_LOG__
        Serial.println("processing disassociation...");
      #endif //__PRINT_LOG__
      processEncryptedFrame(check);
      break;
    }
    case COMMAND_SET_PASSWORD_RESPONSE:
    {
      if (strlen(token) == 0){
        #ifdef __PRINT_LOG__
          Serial.println("no token");
        #endif //__PRINT_LOG__

        print_lcd_message("command failure:","token required");

        sendCommandStatus(COMMAND_PUSH,false);
        return;
      }

      char device_id[8];
      for (int i = 0; i< 8;i++){
        device_id[i] = check[i];
      }

      char * xor_key = config.is_associated(device_id);

      if (xor_key!=0){

        #ifdef __PRINT_LOG__
          Serial.println("device associated, continuing ...");
        #endif //__PRINT_LOG__

        char xored_hash[56];
        for (int i = 8;i < 64;i++){
          xored_hash[i-8] = check[i];
        }

        char xor_decoded[28];

        for (int j = 0 ;j  < 28;j++){
          xor_decoded[j]=xored_hash[j]^xor_key[j];
        }

        #ifdef __PRINT_LOG__
          Serial.println("xor decoded : ");
          for (int i = 0; i < 8;i++){
            Serial.print((int)xor_decoded[i]);
            Serial.print("=>");
            Serial.print((int)token[i]);
            Serial.print(" ");
          }
          Serial.println("");
        #endif //__PRINT_LOG__

        if (memcmp(token,xor_decoded,8)==0){

          memset(token, 0, 8);

          byte key[32];
          for (int i = 0; i < 32;i++){
            key[i]=config.get_config().key[i];
          }

          byte succ = aes.set_key (key, 256);

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
              iv[i] = config.get_config().iv[i] ;
            }
            succ = aes.cbc_encrypt(input, cipher, blocks, iv) ;
          }

          char pass_tmp[64];
          for (int i =0;i<64;i++){
            pass_tmp[i]=cipher[i];
          }
          config.set_pass(pass_tmp);

          #ifdef __PRINT_LOG__
            Serial.println("set password success");
          #endif //__PRINT_LOG__

          save_conf = true;

          print_lcd_message("command status :","set pwd success");

          sendCommandStatus(cmd,true);
        }
        else{
          memset(token, 0, 8);
          #ifdef __PRINT_LOG__
            Serial.println("cmd failure : bad token");
          #endif //__PRINT_LOG__

          print_lcd_message("command failure:","bad token");

          sendCommandStatus(cmd,false);
        }
      }
      else{
        #ifdef __PRINT_LOG__
          Serial.println("device not associated, aborting ...");
        #endif //__PRINT_LOG__

        print_lcd_message("command failure:","not associated");

        sendCommandStatus(cmd,false);
      }
      break;
    }
    case COMMAND_SET_KEYS_RESPONSE:
    {
      #ifdef __PRINT_LOG__
        Serial.println("processing set keys");
      #endif //__PRINT_LOG__

      if (strlen(token) == 0){
        #ifdef __PRINT_LOG__
          Serial.println("no token");
        #endif //__PRINT_LOG__

        print_lcd_message("command failure:","no token");
        sendCommandStatus(COMMAND_PUSH,false);
        return;
      }

      char device_id[8];
      for (int i = 0; i< 8;i++){
        device_id[i] = check[i];
      }

      char * xor_key = config.is_associated(device_id);

      if (xor_key!=0){

        #ifdef __PRINT_LOG__
          Serial.println("device associated, continuing ...");
        #endif //__PRINT_LOG__

        char xored_hash[64];
        for (int i = 8;i < 64;i++){
          xored_hash[i-8] = check[i];
        }

        char xor_decoded[64];

        for (int j = 0 ;j  < 32;j++){
          xor_decoded[j]=xored_hash[j]^xor_key[j];
        }
        for (int j = 32 ;j  < 64;j++){
          xor_decoded[j]=xored_hash[j]^xor_key[j-32];
        }

        #ifdef __PRINT_LOG__
          Serial.println("xor decoded : ");
          for (int i = 0; i < 8;i++){
            Serial.print((int)xor_decoded[i]);
            Serial.print("=>");
            Serial.print((int)token[i]);
            Serial.print(" ");
          }
          Serial.println("");
        #endif //__PRINT_LOG__

        if (memcmp(token,xor_decoded,8)==0){

          //decrypt current pass
          byte pass[64];
          byte iv[16];
          byte cipher[64];
          byte key[32];
          for (int i = 0; i < 32;i++){
            key[i]=config.get_config().key[i];
          }

          byte succ = aes.set_key (key, 256);

          for (byte i = 0 ; i < 16 ; i++){
            iv[i] = config.get_config().iv[i] ;
          }
          for (byte i = 0 ; i< 64;i++){
            cipher[i] = config.get_config().pass[i];
          }
          //decrypt
          succ = aes.cbc_decrypt(cipher, pass, 4, iv) ;

          memset(token, 0, 8);

          #ifdef __PRINT_LOG__
            Serial.println("SET KEYS OK");
          #endif //__PRINT_LOG__

          char key_tmp[32];
          for (int i = 0; i < 32;i++){
            key_tmp[i]=xor_decoded[i+8];
            key[i]=key_tmp[i];
          }
          config.set_key(key_tmp);

          char iv_tmp[16];
          for (int i = 0; i < 16;i++){
            iv_tmp[i]=xor_decoded[i+8+32];
          }
          config.set_iv(iv_tmp);

          succ = aes.set_key (key, 256);

          for (byte i = 0 ; i < N_BLOCK ; i++){
            iv[i] = config.get_config().iv[i] ;
          }
          succ = aes.cbc_encrypt(pass, cipher, 4, iv) ;

          char pass_tmp[64];
          for (byte i = 0; i < 64; i++){
            pass_tmp[i]=cipher[i];
          }
          config.set_pass(pass_tmp);

          #ifdef __PRINT_LOG__
            Serial.println("set key success");
            Serial.println(cmd);
          #endif //__PRINT_LOG__
            
          print_lcd_message("command status :","set keys success");

          save_conf = true;

          sendCommandStatus(cmd,true);
        }
        else{
          memset(token, 0, 8);
          #ifdef __PRINT_LOG__
            Serial.println("cmd failure : bad token");
          #endif //__PRINT_LOG__
          print_lcd_message("command failure:","bad token");
          sendCommandStatus(cmd,false);
        }
      }
      else{
        #ifdef __PRINT_LOG__
          Serial.println("device not associated, aborting ...");
        #endif //__PRINT_LOG__ 
        print_lcd_message("command failure:","not asscociated");
        sendCommandStatus(cmd,false);
      }

      break;
    }
    case COMMAND_ASSOCIATE_RESPONSE:
    {
      #ifdef __PRINT_LOG__
        Serial.println("processing associate response");
      #endif //__PRINT_LOG__ 

      if (strlen(token) == 0){
        sendCommandStatus(COMMAND_ASSOCIATE,false);
        return;
      }

      char device_id[8];

      for (int i = 0; i< 8;i++){
        device_id[i] = check[i];
      }

      char xor_key[32];

      for (int i = 0; i< 32;i++){
        xor_key[i] = check[i+8];
      }

      char token_field[8];

      for (int i = 0; i< 8;i++){
        token_field[i] = check[i+8+32];
      }

      if (memcmp(token,token_field,8)==0){
        
        memset(token, 0, 8);

        #ifdef __PRINT_LOG__
          Serial.println("association success");
        #endif //__PRINT_LOG__ 

        if (external_iv[0]==0x00 || external_key[0]==0x00){
          Serial.println("invalid key/iv");
          sendCommandStatus(COMMAND_ASSOCIATE,false);
          return;
        }
        //record xor / device id
        config.add_device(device_id,xor_key);
        
        byte key_data[64];
        byte iv_data[64];

        for (int i = 0; i < 32;i++){
          key_data[i] = config.get_config().key[i];
        }
        for (int i = 32 ; i < 64;i++){
          key_data[i]= 0;
        }
        for (int i = 0; i < 16;i++){
          iv_data[i] = config.get_config().iv[i];
        }
        for (int i = 16; i < 64;i++){
          iv_data[i] = 0;
        }

        #ifdef __PRINT_LOG__
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
        #endif //__PRINT_LOG__ 

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

        #ifdef __PRINT_LOG__
          Serial.println("DECRYPT KEY");
          for (byte i = 0; i < 64; i++){
            Serial.print (check[i]>>4, HEX) ; Serial.print (check[i]&15, HEX) ; Serial.print (" ") ;
          }
          Serial.println();
        #endif //__PRINT_LOG__ 

        state = STATE_SENDING;
        data_length = 128;
        response = (uint8_t*)malloc(data_length + 1);
        memset(response, 0, data_length + 1);

        for (byte i = 0; i < 64; i++){
          response[i]=cipher[i];
        }

        for (byte i = 0 ; i < N_BLOCK ; i++){
          iv[i] = external_iv[i] ;
        }
        succ = aes.cbc_encrypt(iv_data, cipher, blocks, iv) ;

        for (byte i = 0 ; i < 16 ; i++){
          iv[i] = external_iv[i] ;
        }
        //decrypt
        succ = aes.cbc_decrypt(cipher, check, 4, iv) ;

         for (byte i = 0; i < 64; i++){
          response[i+64]=cipher[i];
        }

        sending_index=0;

        char req[10]={0};
        sprintf(req, "%c:%c:%d", COMMAND_ASSOCIATE_RESPONSE, STATE_SENDING,data_length);
        RFduinoBLE.send(req,10);

      }
      else{
        memset(token, 0, 8);
        #ifdef __PRINT_LOG__
          Serial.println("association failure : not associated");
        #endif //__PRINT_LOG__ 
        print_lcd_message("command failure:","not asscociated");
        sendCommandStatus(COMMAND_ASSOCIATE,false);
      }

      break;
    }
  }
}

void processHeaderFrame(int cmd,char *data, int len){

  #ifdef __PRINT_LOG__
    Serial.println(COMMAND_STRING_ENUM[cmd]);
  #endif //__PRINT_LOG__ 

  if (len>1){
    state = STATE_PROCESSING;
    data_length = data[2] + (data[1]<<8);
    payload = (uint8_t*)malloc(data_length + 1);
    memset(payload, 0, data_length + 1);

    #ifdef __PRINT_LOG__
      Serial.print("cmd: ");
      Serial.println(cmd);
      Serial.print("data_length: ");
      Serial.println(data_length);
    #endif //__PRINT_LOG__
  }
  else{
    #ifdef __PRINT_LOG__
      Serial.print("error length error for ");
      Serial.println(COMMAND_STRING_ENUM[cmd]);
    #endif //__PRINT_LOG__
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

      #ifdef __PRINT_LOG__
        Serial.print("receive command ");
        Serial.print(cmd);
        Serial.print(" ");
      #endif //__PRINT_LOG__

      uint8_t ret = 0;

      switch (cmd){

        case COMMAND_GET_TOKEN:
        {
          #ifdef __PRINT_LOG__
            Serial.println(COMMAND_STRING_ENUM[cmd]);
          #endif //__PRINT_LOG__

          uint16_t lfsr1 = crypto.random();
          uint16_t lfsr2 = crypto.random();
          uint16_t lfsr3 = crypto.random();
          uint16_t lfsr4 = crypto.random();

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

          #ifdef __PRINT_LOG__
            Serial.print("send token : ");
            Serial.println(tokenStr);
          #endif //__PRINT_LOG__

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
        case COMMAND_MESSAGE_BOTTOM:
        case COMMAND_MESSAGE_TOP:
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
        case COMMAND_DISASSOCIATION:
        {
          processHeaderFrame(cmd,data,len);
          break;
        }
        case COMMAND_ASSOCIATE:
        {
          #ifdef __PRINT_LOG__
            Serial.println(COMMAND_STRING_ENUM[cmd]);
          #endif //__PRINT_LOG__

          generateRandomKeys();

          use_local_keys= false;

          char buf[3]={0};
          sprintf(buf, "%c:%c", COMMAND_ASSOCIATE, 2);
          RFduinoBLE.send(buf,3);

          break;
        }
        case COMMAND_RECEIVE_KEYS:
        {
          #ifdef __PRINT_LOG__
            Serial.println("sending keys");
          #endif //__PRINT_LOG__

          char buf[18]={0};
          int diff = data_length-sending_index;

          if (diff<=18){
            
            #ifdef __PRINT_LOG__
              Serial.println("sending last frame");
            #endif //__PRINT_LOG__

            for (int i = 0;i < diff;i++){
              buf[i]=response[i+sending_index];
            }
  
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

      #ifdef __PRINT_LOG__
        Serial.print("length: ");
        Serial.println(data_length);
        Serial.print("offset: ");
        Serial.println(offset);
      #endif //__PRINT_LOG__

      if (offset==data_length){

        #ifdef __PRINT_LOG__
          Serial.println("complete");
          for (int i = 0;i < data_length;i++){
            Serial.print (payload[i]>>4, HEX) ; Serial.print (payload[i]&15, HEX) ; Serial.print (" ") ;
            if ((i!=0) && (i%15==0)){
              Serial.println("");
            }
          }

          Serial.println("");
          Serial.println("decrypting...");
        #endif //__PRINT_LOG__

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

        #ifdef __PRINT_LOG__
          Serial.print("number of block: ");
          Serial.println(block_num);
        #endif //__PRINT_LOG__

        byte iv[N_BLOCK];

        if (use_local_keys){

          for (byte i = 0 ; i < 16 ; i++){
            iv[i] = config.get_config().iv[i] ;
          }

          byte key[32];
          for (int i = 0; i < 32;i++){
            key[i]=config.get_config().key[i];
          }
          succ = aes.set_key (key, 256);
        }
        else{
        
          #ifdef __PRINT_LOG__
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
          #endif //__PRINT_LOG__

          if (external_iv[0]==0x00 || external_key[0]==0x00){
            Serial.println("invalid key/iv");
            sendCommandFailure();
            return;
          }

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