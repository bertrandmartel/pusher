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
#ifndef CRYPTO_H
#define CRYPTO_H

#include "Arduino.h"

class Crypto{

public:
	Crypto();
	~Crypto();
	uint8_t random8();
	uint16_t random16();
	uint16_t random();
	void generateRandomKeys(char * token_str,byte *code);
	void generate_key(byte * code,byte * key);
	void generate_iv(byte * code, byte * iv);

private:
	uint16_t lfsr;
};

#endif //CRYPTO_H