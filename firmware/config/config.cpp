/**********************************************************************************
 * This file is part of Button Pusher.                                             *
 * <p/>                                                                            *
 * Copyright (C) 2016  Bertrand Martel                                             *
 * <p/>                                                                            *
 * Button Pusher is free software: you can redistribute it and/or modify           *
 * it under the terms of the GNU General Public License as published by            *
 * the Free Software Foundation, either version 3 of the License, or               *
 * (at your option) any later version.                                             *
 * <p/>                                                                            *
 * Button Pusher is distributed in the hope that it will be useful,                *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                  *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                   *
 * GNU General Public License for more details.                                    *
 * <p/>                                                                            *
 * You should have received a copy of the GNU General Public License               *
 * along with Button Pusher. If not, see <http://www.gnu.org/licenses/>.           *
 */
#include "config.h"
#include "model.h"
#include "Arduino.h"
#include "aes/AES.h"
#include <RFduinoBLE.h>
#include <lcd/LiquidCrystal.h>
#include "constants.h"

byte default_key[32] = 
{
  0xF2, 0x1E, 0x07, 0x8C, 0x96, 0x99, 0x5E, 0xF7, 0xED, 0xF0, 0x91, 0x84, 0x06, 0x06, 0xF3, 0x94,
  0x59, 0x90, 0x66, 0x63, 0x81, 0xE9, 0x14, 0x3E, 0x7B, 0x02, 0x7E, 0x08, 0xB6, 0xC7, 0x06, 0x26
} ;

byte default_iv[16] = 
{
  0xC3, 0x78, 0x7E, 0x76, 0x31, 0x6D, 0x6B, 0x5B, 0xB8, 0x8E, 0xDA, 0x03, 0x82, 0xEB, 0x57, 0xBD
} ;

byte default_pass[64] = { 
  0x61, 0x64, 0x6d, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

char default_top_message[28] = {
  0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

char default_bottom_message[28] = {
  0x48, 0x61, 0x76, 0x65, 0x20, 0x61, 0x20, 0x67, 0x6f, 0x6f, 0x64, 0x20, 0x64, 0x61, 0x79, 0x21,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

Config::Config(){
	in_flash = (config_t*)ADDRESS_OF_PAGE(CONFIG_STORAGE);
	device_config = ADDRESS_OF_PAGE(DEVICE_CONFIG_STORAGE);
}

Config::~Config(){
}

void Config::init(LiquidCrystal *lcd,AES *aes){
	this->lcd = lcd;
	this->aes = aes;
}

void Config::write_host_config(device *item){

  int rc = flashWriteBlock(device_config, item, sizeof(device));
  device_config+=10;
  
  #ifdef __PRINT_LOG__
    if (rc == 0){
      Serial.println("Success");
    }
    else if (rc == 1){
        Serial.println("Error - the flash page is reserved");
      }
    else if (rc == 2){
        Serial.println("Error - the flash page is used by the sketch");
      }
  #endif //__PRINT_LOG__
}

void Config::save_config(bool default_config){

    #ifdef __PRINT_LOG__
      Serial.println("save_config");
    #endif //__PRINT_LOG__

    if (default_config){

      byte succ = aes->set_key (default_key, 256);

      int blocks = 4;
      byte cipher [64] ;
      byte iv [16];

      if (blocks == 1){
        succ = aes->encrypt(default_pass, cipher) ;
      }
      else {

        for (byte i = 0 ; i < 16 ; i++){
          iv[i] = default_iv[i] ;
        }
        succ = aes->cbc_encrypt(default_pass, cipher, blocks, iv) ;
      }

      for (byte i = 0; i < (4*16); i++){
        config.pass[i]=cipher[i];
      }

      config.flag = 1;
      config.device_num=0;

      for (int i  =0;i< 32;i++){
        config.key[i]=default_key[i];
      }
      for (int i  =0;i< 16;i++){
        config.iv[i]=default_iv[i];
      }
      for (int i = 0; i  < 28;i++){
        config.top_message[i]=default_top_message[i];
      }
      for (int i = 0; i  < 28;i++){
        config.bottom_message[i]=default_bottom_message[i];
      }
    }
    
    while (!RFduinoBLE.radioActive);
    while (RFduinoBLE.radioActive);

    RFduinoBLE.end();

    flashPageErase(PAGE_FROM_ADDRESS(in_flash));

    while (RFduinoBLE_radioActive);

    flashWriteBlock(in_flash, &config, sizeof(config));

    if (add_device_pending){

      #ifdef __PRINT_LOG__
        Serial.println("in add_device_pending");
      #endif //__PRINT_LOG__

      if (config.device_num>0){

        #ifdef __PRINT_LOG__
          Serial.println("saving new associated device");
        #endif //__PRINT_LOG__

        write_host_config(&device_ptr[config.device_num-1]);
      }
    }
}

void Config::add_device(char * device_id,char * xor_key){

  #ifdef __PRINT_LOG__
    Serial.println("adding device in configuration");
  #endif //__PRINT_LOG__

  if (config.device_num<MAX_ASSOCIATED_DEVICE){

    remove_device(device_id);

    #ifdef __PRINT_LOG__
      Serial.println(config.device_num);
    #endif //__PRINT_LOG__

    memcpy(device_ptr[config.device_num].device_id, device_id,8);
    memcpy(device_ptr[config.device_num].xor_key, xor_key,32);

    #ifdef __PRINT_LOG__
      Serial.println("add device to config");
    #endif //__PRINT_LOG__

    config.device_num++;

    add_device_pending=true;

    #ifdef __PRINT_LOG__
      Serial.println(config.device_num);
    #endif //__PRINT_LOG__

  }
  else{

    #ifdef __PRINT_LOG__
      Serial.println("error max device is reached");
    #endif //__PRINT_LOG__

  }
}

void Config::print_all_config(){
  
  uint32_t *save_ptr = device_config;

  save_ptr = ADDRESS_OF_PAGE(DEVICE_CONFIG_STORAGE);

  device *item = 0;

  #ifdef __PRINT_LOG__
    Serial.println("-----------------------------");
    Serial.print("associated device list (size : ");
    Serial.print(config.device_num);
    Serial.println(") : ");
  #endif //__PRINT_LOG__
  
  for (int i = 0; i< config.device_num;i++){
    item = (device*)save_ptr;

    strcpy(device_ptr[i].device_id, item->device_id);
    strcpy(device_ptr[i].xor_key, item->xor_key);

    #ifdef __PRINT_LOG__
      for (int j = 0 ; j < 8;j++){
        Serial.print (device_ptr[i].device_id[j]>>4, HEX) ; Serial.print (device_ptr[i].device_id[j]&15, HEX) ; Serial.print (" ") ;
      }
      Serial.print(" | ");
      for (int j = 0 ; j < 32;j++){
        Serial.print (device_ptr[i].xor_key[j]>>4, HEX) ; Serial.print (device_ptr[i].xor_key[j]&15, HEX) ; Serial.print (" ") ;
      }
      Serial.println("");
    #endif //__PRINT_LOG__

    save_ptr+=10;
  }

  #ifdef __PRINT_LOG__
    Serial.println("-----------------------------");
  #endif //__PRINT_LOG__
}

void Config::erase_host_config(){

  flashPageErase(PAGE_FROM_ADDRESS(device_config));

  for (int i = 0; i< MAX_ASSOCIATED_DEVICE;i++){
    strcpy(device_ptr[i].device_id, "");
    strcpy(device_ptr[i].xor_key, "");
  }
}

char* Config::is_associated(char * device_id){

  for (int i = 0; i< MAX_ASSOCIATED_DEVICE;i++){

    if (memcmp(device_ptr[i].device_id,device_id,8)==0){
      return device_ptr[i].xor_key;
    }
  }
  return 0;
}

void Config::remove_device(char * device_id){

  int index = -1;

  for (int i = 0; i< MAX_ASSOCIATED_DEVICE;i++){

    if (memcmp(device_ptr[i].device_id,device_id,8)==0){
      index=i;
    }
  }
  if ((index!=-1) && (index!=MAX_ASSOCIATED_DEVICE)){
    for (int i = index;i<MAX_ASSOCIATED_DEVICE-1;i++){
      strcpy(device_ptr[i].device_id, device_ptr[i+1].device_id);
      strcpy(device_ptr[i].xor_key  , device_ptr[i+1].xor_key);
    }
    config.device_num--;
    add_device_pending=true;
    print_all_config();
  }
}

void Config::restore(){
 
  in_flash = (config_t*)ADDRESS_OF_PAGE(CONFIG_STORAGE);

  if (in_flash->flag != 0){

    #ifdef __PRINT_LOG__
      Serial.println("writing default configuration");
    #endif //__PRINT_LOG__

    save_config(true);
    
    erase_host_config();

    RFduinoBLE.begin();
  }
  else {

    #ifdef __PRINT_LOG__
      Serial.println("getting default configuration");
    #endif //__PRINT_LOG__

    for (byte i = 0; i < (4*16); i++){
      config.pass[i]=in_flash->pass[i];
    }
    
    config.flag=1;
    config.device_num=in_flash->device_num;

    for (int i = 0; i < 32;i++){
      config.key[i]=in_flash->key[i];
    }

    for (int i = 0; i < 16;i++){
      config.iv[i]=in_flash->iv[i];
    }

    for (int i = 0; i  < 28;i++){
      config.bottom_message[i]=in_flash->bottom_message[i];
    }

    for (int i = 0; i  < 28;i++){
      config.top_message[i]=in_flash->top_message[i];
    }

    #ifdef __PRINT_LOG__
      Serial.print("encrypted password : ");
      for (byte i = 0; i < (4*16); i++){
        Serial.print ( config.pass[i]>>4, HEX) ; Serial.print ( config.pass[i]&15, HEX) ; Serial.print (" ") ;
      }
      Serial.println();
    #endif //__PRINT_LOG__
  }

  print_all_config();
}

config_t Config::get_config(){
	return config;
}

void Config::set_device_pending(bool state){
	add_device_pending=state;
}

bool Config::is_device_pending(){
	return add_device_pending;
}

void Config::set_top_message(char * message){
	for (int i = 0; i < 28;i++){
		config.top_message[i]=message[i];
	}
}

void Config::set_bottom_message(char * message){
	for (int i = 0; i < 28;i++){
		config.bottom_message[i]=message[i];
	}
}

void Config::set_pass(char * pass){
	for (int i = 0; i < 64;i++){
		config.pass[i]=pass[i];
	}
}

void Config::set_key(char * key){
	for (int i = 0; i < 32;i++){
		config.key[i]=key[i];
	}
}

void Config::set_iv(char * iv){
	for (int i = 0; i < 16;i++){
		config.iv[i]=iv[i];
	}
}

bool Config::is_save_conf(){
	return save_conf;
}

void Config::set_save_conf(bool state){
	save_conf=state;
}