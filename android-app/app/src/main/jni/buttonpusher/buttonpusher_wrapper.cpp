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

byte ButtonPusher::key[32]  = 
    {
        0xF2, 0x1E, 0x07, 0x8C, 0x96, 0x99, 0x5E, 0xF7, 0xED, 0xF0, 0x91, 0x84, 0x06, 0x06, 0xF3, 0x94,
        0x59, 0x90, 0x66, 0x63, 0x81, 0xE9, 0x14, 0x3E, 0x7B, 0x02, 0x7E, 0x08, 0xB6, 0xC7, 0x06, 0x26
    } ;

byte ButtonPusher::iv[16]  = 
    {
      0xC3, 0x78, 0x7E, 0x76, 0x31, 0x6D, 0x6B, 0x5B, 0xB8, 0x8E, 0xDA, 0x03, 0x82, 0xEB, 0x57, 0xBD
    } ;

extern "C" {

JNIEXPORT jbyteArray JNICALL Java_com_github_akinaru_roboticbuttonpusher_service_BtPusherService_encrypt(JNIEnv* env, jobject obj,jstring message)
	{
		const char* messageConvert = env->GetStringUTFChars(message, 0);

		int blocks = 4;

		jbyteArray ret = env->NewByteArray(64);

		string messageStr = messageConvert;

		byte payload[64];
		byte cipher [4*N_BLOCK];

		for (int i = 0; i  < messageStr.length();i++){
			payload[i]=messageStr[i];
		}

		for (byte i = messageStr.length(); i < 64; i++){
			payload[i]=0;
		}

		byte succ = 0;

		if (blocks == 1){
			succ = ButtonPusher::aes.encrypt(payload, cipher) ;
		}
		else {

			byte iv_b[N_BLOCK];

			for (int i = 0 ; i < N_BLOCK ; i++){
				iv_b[i] = ButtonPusher::iv[i] ;
			}

			succ = ButtonPusher::aes.cbc_encrypt (payload, cipher, blocks, iv_b) ;
		}
		
		if (succ!=0){
			__android_log_print(ANDROID_LOG_ERROR,"encrypt","encrypt error\n");
		}
		else{
			jbyte payloadBa[64];

			for (byte i = 0; i < 64; i++){
				payloadBa[i]=cipher[i];
				env->SetByteArrayRegion (ret, 0, 64, payloadBa);
			}
		}

		env->ReleaseStringUTFChars(message, messageConvert);
		
		return ret;

	}
}

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* aReserved)
{
	byte succ = ButtonPusher::aes.set_key(ButtonPusher::key, 256);

	if (succ!=0){
		__android_log_print(ANDROID_LOG_ERROR,"JNI_OnLoad","unable to set aes key\n");
	}

	return JNI_VERSION_1_6;
}
}
