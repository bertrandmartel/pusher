#include "LcdPrinter.h"
#include <lcd/LiquidCrystal.h>
#include "config/config.h"

LcdPrinter::LcdPrinter(){
}

LcdPrinter::~LcdPrinter(){
}

void LcdPrinter::init(LiquidCrystal *lcd, Config * config){
	this->lcd = lcd;
	this->config = config;
}

void LcdPrinter::print_lcd_message(char * header,char * message){

  lcd->clear();
  lcd->setCursor(0,0);
  lcd->print(header);
  lcd->setCursor(0,1);
  lcd->print(message);
}

void LcdPrinter::print_default_message(){
  print_lcd_message(config->get_config().top_message,config->get_config().bottom_message);
}
