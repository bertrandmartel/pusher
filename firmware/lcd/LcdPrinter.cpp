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
