#ifndef LCD_PRINTER_H
#define LCD_PRINTER_H

#include <lcd/LiquidCrystal.h>
#include "config/config.h"

class LcdPrinter {

public:

	LcdPrinter();
	~LcdPrinter();
	void init(LiquidCrystal *lcd, Config * config);
	void print_lcd_message(char * header,char * message);
	void print_default_message();

private:
	LiquidCrystal * lcd;
	Config * config;
};

#endif // LCD_PRINTER_H