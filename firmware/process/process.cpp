/**********************************************************************************
 * This file is part of Pusher.                                                    *
 * <p/>                                                                            *
 * Copyright (C) 2016  Bertrand Martel                                             *
 * <p/>                                                                            *
 * Pusher is free software: you can redistribute it and/or modify                  *
 * it under the terms of the GNU General Public License as published by            *
 * the Free Software Foundation, either version 3 of the License, or               *
 * (at your option) any later version.                                             *
 * <p/>                                                                            *
 * Pusher is distributed in the hope that it will be useful,                       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                  *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                   *
 * GNU General Public License for more details.                                    *
 * <p/>                                                                            *
 * You should have received a copy of the GNU General Public License               *
 * along with Pusher. If not, see <http://www.gnu.org/licenses/>.                  *
 */
#include "process.h"
#include "constants.h"
#include "Arduino.h"
#include <RFduinoBLE.h>
#include <Servo.h>
#include <aes/AES.h>
#include "config/config.h"
#include "crypto/crypto.h"
#include "lcd/LcdPrinter.h"

#define SERVO_PIN 2

Process::Process(){
	state = STATE_NONE;
	state_count= 0;
	cmd = 0;
	data_length = 0;
	use_local_keys = true;
	sending_index = 0;
	code_expiring=false;
	code_expiring_time=0;
	offset = 0;
}

Process::~Process(){
}

void Process::set_code_expiring_time(unsigned long time){
  code_expiring_time=time;
}

unsigned long Process::get_code_expiring_time(){
  return code_expiring_time;
}

void Process::set_code_expiring(bool state){
  code_expiring=state;
}

bool Process::is_code_expiring(){
  return code_expiring;
}

void Process::init(LcdPrinter *lcd_printer,AES *aes,Config *config,Crypto *crypto){

	this->crypto = crypto;
	this->config = config;
	this->aes = aes;
	this->lcd_printer = lcd_printer;

  servo.attach(SERVO_PIN);
  servo.write(0);
  servo.detach();
}

void Process::motor_process()
{
  for(int angle = 0; angle < 180; angle++)
  {
    servo.write(angle);
    delay(3);
  }
  delay(500);
  
  for(int angle = 179; angle >0;angle--)
  {
    servo.write(angle);
    delay(5);
  }
}

void Process::clear(){

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
  state=STATE_NONE;
}

void Process::sendCommandStatus(int cmd, bool status){

  char req[2]={0};
  uint8_t status_d = 1;
  if (status){
    status_d=0;
  }
  sprintf(req, "%c:%c", cmd,status_d);
  RFduinoBLE.send(req,10);
}

void Process::sendCommandFailure(){
  char req[1]={0};
  sprintf(req, "%c", COMMAND_FAILURE);
  RFduinoBLE.send(req,2);
}

void Process::processEncryptedResponse(byte * check){

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

void Process::processEncryptedFrame(byte * check){

  if (strlen(token) == 0){

    lcd_printer->print_lcd_message("command failure:","no token sent");

    sendCommandStatus(COMMAND_PUSH,false);
    return;
  }

  char device_id[8];

  for (int i = 0; i< 8;i++){
    device_id[i] = check[i];
  }

  char * xor_key = config->is_associated(device_id);

  if (xor_key!=0){

    char xored_hash[28];
    for (int i = 8;i < 36;i++){
      xored_hash[i-8] = check[i];
    }

    char xor_decoded[28];

    
    for (int j = 0 ;j  < 28;j++){
      xor_decoded[j]=xored_hash[j]^xor_key[j];
    }

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
        key[i]=config->get_config().key[i];
      }
      byte succ = aes->set_key (key, 256);

      for (byte i = 0 ; i < 16 ; i++){
        iv[i] = config->get_config().iv[i] ;
      }
      for (byte i = 0 ; i< 64;i++){
        cipher[i] = config->get_config().pass[i];
      }
      //decrypt
      succ = aes->cbc_decrypt(cipher, pass, 4, iv) ;
      
      for (int i  =0; i  < 64;i++){
        pass_temp[i]=pass[i];
      }

      if (memcmp(sent_pass,pass_temp,64)==0){

        switch (cmd){
          case COMMAND_PUSH:
          {
            lcd_printer->print_lcd_message("command status :","push success");

            sendCommandStatus(COMMAND_PUSH,true);
            servo.attach(SERVO_PIN);
            motor_process();
            servo.detach();
            break;
          }
          case COMMAND_MESSAGE_TOP:
          {
            char top_message[28];
            for (int i = 36;i < 64;i++){
              top_message[i-36] = check[i];
            }
            config->set_top_message(top_message);
            config->set_save_conf(true);

            sendCommandStatus(cmd,true);
            break;
          }
          case COMMAND_MESSAGE_BOTTOM:
          {
            char bottom_message[28];
            for (int i = 36;i < 64;i++){
              bottom_message[i-36] = check[i];
            }
            config->set_bottom_message(bottom_message);
            config->set_save_conf(true);

            sendCommandStatus(cmd,true);
            break;
          }
          case COMMAND_SET_PASSWORD:
          {
            use_local_keys= false;

            /*-------------------------*/
            char token_str[16];
            byte code[8];
            crypto->generateRandomKeys(token_str,code);
            crypto->generate_key(code,external_key);
            crypto->generate_iv(code,external_iv);

            code_expiring_time=millis()+CODE_EXPIRING_TIME_SECONDS*1000;
            code_expiring=true;

            lcd_printer->print_lcd_message("pairing code :",token_str);
            /*-------------------------*/

            char buf[3]={0};
            sprintf(buf, "%c:%c", COMMAND_SET_PASSWORD, 2);
            RFduinoBLE.send(buf,3);
            break;
          }
          case COMMAND_SET_KEY:
          {
            /*-------------------------*/
            char token_str[16];
            byte code[8];
            crypto->generateRandomKeys(token_str,code);
            crypto->generate_key(code,external_key);
            crypto->generate_iv(code,external_iv);

            code_expiring_time=millis()+CODE_EXPIRING_TIME_SECONDS*1000;
            code_expiring=true;

            lcd_printer->print_lcd_message("pairing code :",token_str);
            /*-------------------------*/

            use_local_keys= false;
            char buf[3]={0};
            sprintf(buf, "%c:%c", COMMAND_SET_KEY, 2);
            RFduinoBLE.send(buf,3);
            break;
          }
          case COMMAND_DISASSOCIATION:
          {
            lcd_printer->print_lcd_message("command status :","unpairing OK");

            config->remove_device(device_id);
            char buf[3]={0};
            sprintf(buf, "%c:%c", COMMAND_DISASSOCIATION, 0);
            RFduinoBLE.send(buf,3);
          }
        }
      }
      else{
        lcd_printer->print_lcd_message("command failure :","bad password");
        delay(500);
        sendCommandStatus(cmd,false);
      } 
    }
    else{
      lcd_printer->print_lcd_message("command failure:","bad token");
      delay(500);
      memset(token, 0, 8);
      sendCommandStatus(cmd,false);
    }
  }
  else{
    lcd_printer->print_lcd_message("command failure:","not associated");
    sendCommandStatus(cmd,false);
  }
}

void Process::executeCmd(byte *check){

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

      char * xor_key = config->is_associated(device_id);

      if (xor_key!=0){

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
          sendCommandStatus(COMMAND_ASSOCIATION_STATUS,true);
        }
        else{
          memset(token, 0, 8);
          sendCommandStatus(COMMAND_ASSOCIATION_STATUS,false);
        }
      }
      else{
        sendCommandStatus(COMMAND_ASSOCIATION_STATUS,false);
      }
      break;
    }
    case COMMAND_PUSH:
    case COMMAND_MESSAGE_BOTTOM:
    case COMMAND_MESSAGE_TOP:
    case COMMAND_SET_PASSWORD:
    case COMMAND_SET_KEY:
    case COMMAND_DISASSOCIATION:
    {
      processEncryptedFrame(check);
      break;
    }
    case COMMAND_SET_PASSWORD_RESPONSE:
    {
      if (strlen(token) == 0){

        lcd_printer->print_lcd_message("command failure:","token required");
        delay(500);
        sendCommandStatus(COMMAND_PUSH,false);
        return;
      }

      char device_id[8];
      for (int i = 0; i< 8;i++){
        device_id[i] = check[i];
      }

      char * xor_key = config->is_associated(device_id);

      if (xor_key!=0){

        char xored_hash[56];
        for (int i = 8;i < 64;i++){
          xored_hash[i-8] = check[i];
        }

        char xor_decoded[28];

        for (int j = 0 ;j  < 28;j++){
          xor_decoded[j]=xored_hash[j]^xor_key[j];
        }

        if (memcmp(token,xor_decoded,8)==0){

          memset(token, 0, 8);

          byte key[32];
          for (int i = 0; i < 32;i++){
            key[i]=config->get_config().key[i];
          }

          byte succ = aes->set_key (key, 256);

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
            succ = aes->encrypt(input, cipher) ;
          }
          else {

            for (byte i = 0 ; i < N_BLOCK ; i++){
              iv[i] = config->get_config().iv[i] ;
            }
            succ = aes->cbc_encrypt(input, cipher, blocks, iv) ;
          }

          char pass_tmp[64];
          for (int i =0;i<64;i++){
            pass_tmp[i]=cipher[i];
          }
          config->set_pass(pass_tmp);

          config->set_save_conf(true);

          lcd_printer->print_lcd_message("command status :","set pwd success");
          delay(500);
          sendCommandStatus(cmd,true);
        }
        else{

          memset(token, 0, 8);
          lcd_printer->print_lcd_message("command failure:","bad token");
          delay(500);
          sendCommandStatus(cmd,false);
        }
      }
      else{

        lcd_printer->print_lcd_message("command failure:","not associated");
        sendCommandStatus(cmd,false);
      }
      break;
    }
    case COMMAND_SET_KEYS_RESPONSE:
    {
      if (strlen(token) == 0){

        lcd_printer->print_lcd_message("command failure:","no token");
        delay(500);
        sendCommandStatus(COMMAND_PUSH,false);
        return;
      }

      char device_id[8];
      for (int i = 0; i< 8;i++){
        device_id[i] = check[i];
      }

      char * xor_key = config->is_associated(device_id);

      if (xor_key!=0){

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

        if (memcmp(token,xor_decoded,8)==0){

          //decrypt current pass
          byte pass[64];
          byte iv[16];
          byte cipher[64];
          byte key[32];
          for (int i = 0; i < 32;i++){
            key[i]=config->get_config().key[i];
          }

          byte succ = aes->set_key (key, 256);

          for (byte i = 0 ; i < 16 ; i++){
            iv[i] = config->get_config().iv[i] ;
          }
          for (byte i = 0 ; i< 64;i++){
            cipher[i] = config->get_config().pass[i];
          }
          //decrypt
          succ = aes->cbc_decrypt(cipher, pass, 4, iv) ;

          memset(token, 0, 8);

          char key_tmp[32];
          for (int i = 0; i < 32;i++){
            key_tmp[i]=xor_decoded[i+8];
            key[i]=key_tmp[i];
          }
          config->set_key(key_tmp);

          char iv_tmp[16];
          for (int i = 0; i < 16;i++){
            iv_tmp[i]=xor_decoded[i+8+32];
          }
          config->set_iv(iv_tmp);

          succ = aes->set_key (key, 256);

          for (byte i = 0 ; i < N_BLOCK ; i++){
            iv[i] = config->get_config().iv[i] ;
          }
          succ = aes->cbc_encrypt(pass, cipher, 4, iv) ;

          char pass_tmp[64];
          for (byte i = 0; i < 64; i++){
            pass_tmp[i]=cipher[i];
          }
          config->set_pass(pass_tmp);

          lcd_printer->print_lcd_message("command status :","set keys success");
          delay(500);
          config->set_save_conf(true);
          sendCommandStatus(cmd,true);
        }
        else{
          memset(token, 0, 8);
          lcd_printer->print_lcd_message("command failure:","bad token");
          delay(500);
          sendCommandStatus(cmd,false);
        }
      }
      else{
        lcd_printer->print_lcd_message("command failure:","not asscociated");
        sendCommandStatus(cmd,false);
      }

      break;
    }
    case COMMAND_ASSOCIATE_RESPONSE:
    {

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

        if (external_iv[0]==0x00 || external_key[0]==0x00){
          sendCommandStatus(COMMAND_ASSOCIATE,false);
          return;
        }

        //record xor / device id
        config->add_device(device_id,xor_key);
        
        byte key_data[64];
        byte iv_data[64];

        for (int i = 0; i < 32;i++){
          key_data[i] = config->get_config().key[i];
        }
        for (int i = 32 ; i < 64;i++){
          key_data[i]= 0;
        }
        for (int i = 0; i < 16;i++){
          iv_data[i] = config->get_config().iv[i];
        }
        for (int i = 16; i < 64;i++){
          iv_data[i] = 0;
        }

        byte succ = aes->set_key (external_key, 256);

        int blocks = 4;
        byte cipher [64] ;
        byte iv [16];
        for (byte i = 0 ; i < 16 ; i++){
          iv[i] = external_iv[i] ;
        }
        succ = aes->cbc_encrypt(key_data, cipher, blocks, iv) ;

        byte check[64];
        for (byte i = 0 ; i < 16 ; i++){
          iv[i] = external_iv[i] ;
        }
        //decrypt
        succ = aes->cbc_decrypt(cipher, check, 4, iv) ;

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
        succ = aes->cbc_encrypt(iv_data, cipher, blocks, iv) ;

        for (byte i = 0 ; i < 16 ; i++){
          iv[i] = external_iv[i] ;
        }
        //decrypt
        succ = aes->cbc_decrypt(cipher, check, 4, iv) ;

         for (byte i = 0; i < 64; i++){
          response[i+64]=cipher[i];
        }

        sending_index=0;

        lcd_printer->print_lcd_message("command success:","associated");
        delay(500);
        char req[10]={0};
        sprintf(req, "%c:%c:%d", COMMAND_ASSOCIATE_RESPONSE, STATE_SENDING,data_length);
        RFduinoBLE.send(req,10);

      }
      else{
        memset(token, 0, 8);
        lcd_printer->print_lcd_message("command failure:","not asscociated");
        sendCommandStatus(COMMAND_ASSOCIATE,false);
      }

      break;
    }
  }
}

void Process::processHeaderFrame(int cmd,char *data, int len){

  #ifdef __PRINT_LOG__
    Serial.println(COMMAND_STRING_ENUM[cmd]);
  #endif //__PRINT_LOG__ 

  if (len>1){
    state = STATE_PROCESSING;
    data_length = data[2] + (data[1]<<8);
    payload = (uint8_t*)malloc(data_length + 1);
    memset(payload, 0, data_length + 1);
  }
}

void Process::process_state(char *data, int len){

  switch (state){

    case STATE_NONE:
    {
      free(payload);
      payload = 0;
      offset=0;
    
      cmd = data[0];
      
      uint8_t ret = 0;

      switch (cmd){

        case COMMAND_GET_TOKEN:
        {
          uint16_t lfsr1 = crypto->random();
          uint16_t lfsr2 = crypto->random();
          uint16_t lfsr3 = crypto->random();
          uint16_t lfsr4 = crypto->random();

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

          ret=1;
          RFduinoBLE.send(buf,20);
          break;
        }
        case COMMAND_ASSOCIATE_RESPONSE:
        {
          processHeaderFrame(cmd,data,len);
          break;
        }
        case COMMAND_ASSOCIATION_STATUS:
        case COMMAND_PUSH:
        case COMMAND_MESSAGE_BOTTOM:
        case COMMAND_MESSAGE_TOP:
        case COMMAND_SET_PASSWORD_RESPONSE:
        case COMMAND_SET_KEYS_RESPONSE:
        case COMMAND_SET_PASSWORD:
        case COMMAND_SET_KEY:
        case COMMAND_DISASSOCIATION:
        {
          processHeaderFrame(cmd,data,len);
          break;
        }
        case COMMAND_ASSOCIATE:
        {
          use_local_keys= false;
          
          /*-------------------------*/
          char token_str[16];
          byte code[8];
          crypto->generateRandomKeys(token_str,code);
          crypto->generate_key(code,external_key);
          crypto->generate_iv(code,external_iv);

          code_expiring_time=millis()+CODE_EXPIRING_TIME_SECONDS*1000;
          code_expiring=true;

          lcd_printer->print_lcd_message("pairing code :",token_str);
          /*-------------------------*/

          char buf[3]={0};
          sprintf(buf, "%c:%c", COMMAND_ASSOCIATE, 2);
          RFduinoBLE.send(buf,3);

          break;
        }
        case COMMAND_RECEIVE_KEYS:
        {
          char buf[18]={0};
          int diff = data_length-sending_index;

          if (diff<=18){

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

      if (offset==data_length){

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

        byte iv[N_BLOCK];
      
        if (use_local_keys){

          for (byte i = 0 ; i < 16 ; i++){
            iv[i] = config->get_config().iv[i] ;
          }

          byte key[32];
          for (int i = 0; i < 32;i++){
            key[i]=config->get_config().key[i];
          }
          succ = aes->set_key (key, 256);
        }
        else{

          if (external_iv[0]==0x00 || external_key[0]==0x00){
            lcd_printer->print_lcd_message("FAILURE","");
            sendCommandFailure();
            return;
          }

          succ = aes->set_key (external_key, 256);

          for (byte i = 0 ; i < 16 ; i++){
            iv[i] = external_iv[i] ;
          }
          use_local_keys=true;
        }

        //decrypt
        succ = aes->cbc_decrypt(cipher, check, 4, iv) ;

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