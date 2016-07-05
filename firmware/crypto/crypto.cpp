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
#include "crypto.h"
#include "Arduino.h"

Crypto::Crypto(){
	lfsr = random16();
}

Crypto::~Crypto(){

}

uint8_t Crypto::random8(){

  uint8_t __value;

  NRF_RNG->TASKS_START = 1; // Turn on the RNG.

  NRF_RNG->EVENTS_VALRDY = 0; // wait for a RNG value to be available

  while( NRF_RNG->EVENTS_VALRDY == 0 ){}
  __value = (uint8_t)NRF_RNG->VALUE; // read the RNG value

  NRF_RNG->TASKS_STOP = 1; // Turn off the RNG if we don't need it much.

  return(__value);
}

uint16_t Crypto::random16(){
  return(random8() * random8());
}

uint16_t Crypto::random()
{
  uint16_t bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
  return lfsr =  (lfsr >> 1) | (bit << 15);
}

void Crypto::generateRandomKeys(char * token_str,byte *code){

  //generate a key from 8 octet random
  uint16_t lfsr1 = random();
  uint16_t lfsr2 = random();
  uint16_t lfsr3 = random();
  uint16_t lfsr4 = random();

  sprintf(token_str, "%04x%04x%04x%04x", lfsr1, lfsr2,lfsr3,lfsr4);
  
  code[0]=((lfsr1 & 0xFF00)>>8);
  code[1]=((lfsr1 & 0x00FF)>>0);
  code[2]=((lfsr2 & 0xFF00)>>8);
  code[3]=((lfsr2 & 0x00FF)>>0);
  code[4]=((lfsr3 & 0xFF00)>>8);
  code[5]=((lfsr3 & 0x00FF)>>0);
  code[6]=((lfsr4 & 0xFF00)>>8);
  code[7]=((lfsr4 & 0x00FF)>>0);
}

void Crypto::generate_key(byte * code,byte * key){
  
  uint16_t lfsr_tmp;

  lfsr_tmp = (code[0]<<8) + code[1];

  uint8_t j = 2;
  uint8_t k = 0;

  for (int i = 0; i  < 16;i++){
    
    if (i!=0 && ((i%4)==0)){

      lfsr_tmp = (code[j]<<8) + code[j+1];
      j+=2;
    }

    
    uint16_t bit  = ((lfsr_tmp >> 0) ^ (lfsr_tmp >> 2) ^ (lfsr_tmp >> 3) ^ (lfsr_tmp >> 5) ) & 1;
    lfsr_tmp =  (lfsr_tmp >> 1) | (bit << 15);

    key[k] = (lfsr_tmp & 0xFF00)>>8;
    k++;
    key[k] = (lfsr_tmp & 0x00FF)>>0;
    k++;
    
  }
}

void Crypto::generate_iv(byte * code, byte * iv){

  for (int i = 0; i  < 8;i++){
    iv[i] = code[i];
  }

  uint16_t lfsr_tmp;
  uint8_t k = 0;
  uint8_t j = 0;

  for (int i = 0; i  < 4;i++){
    lfsr_tmp = (code[j]<<8) + code[j+1];
    j+=2;
    uint16_t bit  = ((lfsr_tmp >> 0) ^ (lfsr_tmp >> 2) ^ (lfsr_tmp >> 3) ^ (lfsr_tmp >> 5) ) & 1;
    lfsr_tmp =  (lfsr_tmp >> 1) | (bit << 15);
    iv[k+8] = (lfsr_tmp & 0xFF00)>>8;
    k++;
    iv[k+8] = (lfsr_tmp & 0x00FF)>>0;
    k++;
  }
}