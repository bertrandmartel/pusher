#include "Arduino.h"
#include <RFduinoBLE.h>
#include <lcd/LiquidCrystal.h>
#include <aes/AES.h>
#include "config/config.h"
#include "crypto/crypto.h"
#include "constants.h"
#include "process/process.h"
#include "lcd/LcdPrinter.h"

Crypto crypto;
Config config;
AES aes;
LcdPrinter lcd_printer;

Process process;
LiquidCrystal lcd(6, 5, 4, 3, 1, 0);

void setup() {

  #ifdef __PRINT_LOG__
  Serial.begin(9600);
  #endif //__PRINT_LOG__
  
  lcd.begin(16, 2);

  config.init(&lcd, &aes);
  lcd_printer.init(&lcd,&config);
  process.init(&lcd_printer,&aes,&config,&crypto);

  RFduinoBLE.advertisementData = ADVERTISMENT_DATA;

  RFduinoBLE.begin();

  config.restore();

  lcd_printer.print_default_message();
}

bool interrupting(){

  if(RFduino_pinWoke(INTERRUPT_PIN)){

    RFduino_resetPinWake(INTERRUPT_PIN);

    return HIGH;
  }
  return LOW;
}

void loop() {

  delay(500);
  
  if(interrupting()){

    #ifdef __PRINT_LOG__
      Serial.println("interrupting");
    #endif //__PRINT_LOG__
      

    if (config.is_save_conf() || config.is_device_pending()){
      config.save_config(false);
      RFduinoBLE.begin();
    }
    
    config.set_save_conf(false);
    config.set_device_pending(false);


    lcd_printer.print_default_message();
  }
  
  if (process.is_code_expiring()){

    int rem = process.get_code_expiring_time()-millis();

    if (process.get_code_expiring_time()!=0 && rem<=0){
      
      #ifdef __PRINT_LOG__
      Serial.println("code has expired");
      #endif //__PRINT_LOG__

      process.clear();

      lcd_printer.print_default_message();
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

void RFduinoBLE_onDisconnect(){

  #ifdef __PRINT_LOG__
  Serial.println("onDisconnect");
  #endif //__PRINT_LOG__
  
  process.clear();

  interrupt();
}

void RFduinoBLE_onReceive(char *data, int len){
  
  process.process_state(data,len);

}

int main() {

  init();
  setup();
  while(1)
    loop();
  return 0;
}