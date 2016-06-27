#ifndef MODEL_H
#define MODEL_H

#include "Arduino.h"

struct device
{
  char device_id[8];
  char xor_key[32];
};

struct config_t
{
  char pass[4*16];
  uint16_t flag;
  uint8_t device_num;
  char key[32];
  char iv[16];
  char top_message[28];
  char bottom_message[28];
};

#endif // MODEL_H