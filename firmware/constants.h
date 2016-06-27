#ifndef CONSTANTS_H
#define CONSTANTS_H

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

#define MAX_ASSOCIATED_DEVICE 25
#define CODE_EXPIRING_TIME_SECONDS 120
#define CONFIG_STORAGE 251
#define DEVICE_CONFIG_STORAGE 250
#define INTERRUPT_PIN 31
#define ADVERTISMENT_DATA "open LAB"

#endif //CONSTANTS_H