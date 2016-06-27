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