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
#ifndef CONFIG_H
#define CONFIG_H

#include "model.h"
#include "aes/AES.h"
#include <lcd/LiquidCrystal.h>

class Config{

public:

	Config();
	~Config();
	void init(LiquidCrystal *lcd,AES *aes);
	void write_host_config();
	void save_config(bool default_config);
	void add_device(char * device_id,char * xor_key);
	void print_all_config();
	void erase_host_config();
	char* is_associated(char * device_id);
	void remove_device(char * device_id);
	void restore();
	config_t get_config();
	bool is_device_pending();
	void set_device_pending(bool state);
	void set_top_message(char * message);
	void set_bottom_message(char * message);
	void set_pass(char * pass);
	void set_key(char * key);
	void set_iv(char * iv);
	bool is_save_conf();
	void set_save_conf(bool state);

private:

	LiquidCrystal *lcd;
	AES *aes;
	config_t *in_flash;
	uint32_t *device_config;
	bool save_conf = false;
	bool add_device_pending = false;
	device * device_ptr = new device[25];
	config_t config = { 
	  "admin" ,
	  0,
	  0,
	  {
	    0xF2, 0x1E, 0x07, 0x8C, 0x96, 0x99, 0x5E, 0xF7, 0xED, 0xF0, 0x91, 0x84, 0x06, 0x06, 0xF3, 0x94,
	    0x59, 0x90, 0x66, 0x63, 0x81, 0xE9, 0x14, 0x3E, 0x7B, 0x02, 0x7E, 0x08, 0xB6, 0xC7, 0x06, 0x26
	  },
	  {
	    0xC3, 0x78, 0x7E, 0x76, 0x31, 0x6D, 0x6B, 0x5B, 0xB8, 0x8E, 0xDA, 0x03, 0x82, 0xEB, 0x57, 0xBD
	  }
	};
};

#endif //CONFIG_H