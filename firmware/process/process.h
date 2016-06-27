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
#ifndef PROCESS_H
#define PROCESS_H

#include "Arduino.h"
#include <Servo.h>
#include <aes/AES.h>
#include "config/config.h"
#include "crypto/crypto.h"
#include "lcd/LcdPrinter.h"

class Process{

public:

	Process();
	~Process();
	void init(LcdPrinter *lcd_printer,AES *aes,Config *config,Crypto *crypto);
	void sendCommandStatus(int cmd, bool status);
	void sendCommandFailure();
	void processEncryptedResponse(byte * check);
	void processEncryptedFrame(byte * check);
	void executeCmd(byte *check);
	void processHeaderFrame(int cmd,char *data, int len);
	void process_state(char *data, int len);
	void clear();
	void set_code_expiring_time(unsigned long time);
	void set_code_expiring(bool state);
	bool is_code_expiring();
	unsigned long get_code_expiring_time();
	void motor_process();

private:

	Servo servo;
	Crypto *crypto;
	Config *config;
	AES *aes;
	LcdPrinter *lcd_printer;
	char token[16];
	uint8_t state;
	uint8_t state_count;
	uint8_t cmd ;
	uint16_t data_length;
	bool use_local_keys;
	uint16_t sending_index;
	bool code_expiring;
	unsigned long code_expiring_time;
	byte external_key[32];
	byte external_iv[16];
	uint8_t* payload;
	uint8_t* response;
	uint8_t offset;
};

#endif // PROCESS_H