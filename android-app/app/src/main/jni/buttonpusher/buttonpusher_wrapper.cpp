
/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Bertrand Martel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/**
	buttonpusher_wrapper.cpp

	Android wrapper for button pusher

	@author Bertrand Martel
	@version 0.1
*/
#include <string>
#include "AES.h"
#include "android/log.h"
#include <jni.h>
#include "buttonpusher.h"

using namespace std;

AES  ButtonPusher::aes;

extern "C" {

JNIEXPORT jbyteArray JNICALL Java_com_github_akinaru_roboticbuttonpusher_service_BtPusherService_generatekey(JNIEnv* env, 
	jobject obj,jbyteArray code)
{

	jbyte *code_b = (jbyte *)env->GetByteArrayElements(code, NULL);

	jbyteArray ret = env->NewByteArray(32);

	jbyte key[32];

	uint16_t lfsr;

	lfsr = ((code_b[0] & 0xFF)<<8) + (code_b[1] & 0xFF);

	uint8_t j = 2;
	uint8_t k = 0;

	for (int i = 0; i  < 16;i++){

		if (i!=0 && ((i%4)==0)){
			lfsr = ((code_b[j] & 0xFF)<<8) + (code_b[j+1] & 0xFF);
			j+=2;
		}
		uint16_t bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
		lfsr =  (lfsr >> 1) | (bit << 15);

		key[k] = (lfsr & 0xFF00)>>8;
		k++;
		key[k] = (lfsr & 0x00FF)>>0;
		k++;
	}

	env->SetByteArrayRegion (ret, 0, 32, key);

	env->ReleaseByteArrayElements(code, code_b, 0 );

	return ret;
}
}

extern "C" {

JNIEXPORT jbyteArray JNICALL Java_com_github_akinaru_roboticbuttonpusher_service_BtPusherService_generateiv(JNIEnv* env, 
	jobject obj,jbyteArray code)
{

	jbyte *code_b = (jbyte *)env->GetByteArrayElements(code, NULL);

	jbyteArray ret = env->NewByteArray(16);

	jbyte iv[16];

	for (int i = 0; i  < 8;i++){
		iv[i] = (code_b[i] & 0xFF);
	}

	uint16_t lfsr;
	uint8_t k = 0;
	uint8_t j = 0;

	for (int i = 0; i  < 4;i++){
		lfsr = ((code_b[j] & 0xFF)<<8) + (code_b[j+1] & 0xFF);
		j+=2;
		uint16_t bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
		lfsr =  (lfsr >> 1) | (bit << 15);
		iv[k+8] = (lfsr & 0xFF00)>>8;
		k++;
		iv[k+8] = (lfsr & 0x00FF)>>0;
		k++;
	}

	env->SetByteArrayRegion (ret, 0, 16, iv);

	env->ReleaseByteArrayElements(code, code_b, 0 );

	return ret;
}
}

extern "C" {

JNIEXPORT jbyteArray JNICALL Java_com_github_akinaru_roboticbuttonpusher_service_BtPusherService_encrypt(JNIEnv* env, 
	jobject obj,jbyteArray message,jint length,jbyteArray key,jbyteArray iv)
	{

		jbyte *message_b = (jbyte *)env->GetByteArrayElements(message, NULL);
		jbyte *key_ba = (jbyte *)env->GetByteArrayElements(key, NULL);
		jbyte *iv_ba = (jbyte *)env->GetByteArrayElements(iv, NULL);

		byte * cData = (byte*)message_b;
		byte * key_c = (byte*)key_ba;
		byte * iv_c = (byte*)iv_ba;

		int message_length = (int)length;

		byte succ = ButtonPusher::aes.set_key(key_c, 256);

		if (succ!=0){
			__android_log_print(ANDROID_LOG_ERROR,"JNI_OnLoad","unable to set aes key\n");
		}

		int blocks = 4;

		jbyteArray ret = env->NewByteArray(64);

		byte payload[64];
		byte cipher [4*N_BLOCK];

		for (int i = 0; i  < message_length;i++){
			payload[i]=cData[i];
		}

		for (byte i = message_length; i < 64; i++){
			payload[i]=0;
		}

		if (blocks == 1){
			succ = ButtonPusher::aes.encrypt(payload, cipher) ;
		}
		else {
			/*
			byte iv_b[N_BLOCK];

			for (int i = 0 ; i < N_BLOCK ; i++){
				iv_b[i] = iv_c[i] ;
			}
			*/

			succ = ButtonPusher::aes.cbc_encrypt (payload, cipher, blocks, iv_c) ;
		}
		
		if (succ!=0){
			__android_log_print(ANDROID_LOG_ERROR,"encrypt","encrypt error\n");
		}
		else{
			jbyte payloadBa[64];

			for (byte i = 0; i < 64; i++){
				payloadBa[i]=cipher[i];
			}
			env->SetByteArrayRegion (ret, 0, 64, payloadBa);
		}

		env->ReleaseByteArrayElements(message, message_b, 0 );
		env->ReleaseByteArrayElements(key, key_ba, 0 );
		env->ReleaseByteArrayElements(iv, iv_ba, 0 );

		return ret;

	}
}

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* aReserved)
{
	return JNI_VERSION_1_6;
}
}
