/*
 * NdefMessage.cpp
 *
 *  Created on: 2013. 12. 27.
 *      Author: innocentevil
 */
#include "NdefMessage.h"
#include <mem.h>

#define DBG 0


namespace NFC {

#define LOGE(x) {Serial.print(F("Error @ NdefMessage : "));Serial.println(F(x));}
#define LOGEN(x,r) {Serial.print(F("Error @ NdefMessage : "));Serial.print(F(x));Serial.println(r,HEX);}
#define LOGD(x) {Serial.print(F("Debug @ NdefMessage : "));Serial.println(F(x));}
#define LOGDN(x,r) {Serial.print(F("Debug @ NdefMessage : "));Serial.print(F(x));Serial.println(r,HEX);}
#define PRINT_ARRAY(l,x,s) {Serial.print("\n");Serial.print(F(l));for(int i = 0;i < s;i++){Serial.print("\t");Serial.print(x[i],HEX);}Serial.print("\n");}
#define NDEF_FILE_HEADER_SIZE 2  //NLEN Filed Size : 2 bytes   *NLEN = size of NDEF Message
#define UBYTE(x) (uint8_t)(x >> 8)
#define LBYTE(x) (uint8_t) x

NdefFile::NdefFile(NdefMessage& msg) {
	uint16_t offsetCounter = 0;
	size = msg.getSizeInByte();
	fileimg = new uint8_t[size + NDEF_FILE_HEADER_SIZE]; //prepare byte array to construct file image into it

	fileimg[0] = UBYTE(size);
	fileimg[1] = LBYTE(size);
	offsetCounter += 2; // fill NLEN field and increase offset

	//write Payload into file image
	offsetCounter += msg.write(&fileimg[offsetCounter]); // fill NDEF Message into file image (byte array)

	EoF = (uint8_t*) fileimg + offsetCounter; //Pointer of EoF
	cPos = fileimg; // pointer used to access file is set to point at entry of file image
#if DBG
	LOGDN("Total Size of File Image : ", offsetCounter);
	LOGDN("NDEF  Size of File       : ", size);
#endif

}

NdefFile::~NdefFile() {
	if (fileimg != NULL) {
		delete[] fileimg;
	}
}

uint32_t NdefFile::read(uint8_t* rbuf, uint16_t size) {
	uint8_t rdSize = 0;
	uint8_t* toread = cPos + size;
	for (; (cPos < toread); cPos++, rdSize++) {
		if (cPos < EoF) {
			rbuf[rdSize] = *cPos; // copy file image into read buffer.
		} else {
			rbuf[rdSize] = 0; // read request beyond EoF will be zero-padded.
		}
	}
#if DBG
	PRINT_ARRAY("Read Data : ", rbuf, rdSize);
#endif
	return rdSize;
}

void NdefFile::seek(uint16_t offset) {
	cPos = (uint8_t*) fileimg + offset;
}

uint32_t NdefFile::write(NdefMessage* msg) {
	delete[] fileimg; //clear fileimg
	uint32_t offset = 0;
	size = msg->getSizeInByte();
	fileimg = new uint8_t[size + NDEF_FILE_HEADER_SIZE];
	cPos = fileimg;
	fileimg[0] = UBYTE(size);
	fileimg[1] = LBYTE(size);
	offset += 2; // fill NLEN Field and increase offset
	offset += msg->write(&fileimg[offset]);
	EoF = fileimg + offset;
	return offset;
}

void NdefFile::print() {
	LOGDN("NLEN (Size of NDEF Message in Byte) : ",size);
	uint32_t offset = 0;
	for(;fileimg + offset < EoF;offset++){
		Serial.print("Value @ ");Serial.print(offset,DEC);LOGDN(" : ",fileimg[offset]);
	}
}

uint32_t NdefFile::getSizeInByte() {
	return size;
}

NdefMessage::NdefMessage(NdefRecord** rcds, int rsize) {
	this->rcds = new NdefRecord*[rsize];
	this->size = rsize;
	for (int i = 0; i < rsize; i++) {
		this->rcds[i] = rcds[i];
	}
	(*this->rcds[0]).setFlag(NDEF_FLAG_MSK_MB);
	(*this->rcds[size - 1]).setFlag(NDEF_FLAG_MSK_ME);
}

NdefMessage::NdefMessage() {
	this->rcds = new NdefRecord*;
	(*this->rcds) = NdefRecord::createEmptyRecord();	// create empty NDEF record
	size = 1;
}

NdefMessage::NdefMessage(const NdefMessage& org) {
	this->rcds = new NdefRecord*[org.size];
	this->size = org.size;
	for (int i = 0; i < this->size; i++) {
		this->rcds[i] = org.rcds[i];
	}
}

NdefMessage::~NdefMessage() {
	if (rcds != NULL) {
		int i = 0;
		for(;i <size;i++){
			if(rcds[i] != NULL){
				delete rcds[i];
			}
		}	//free allocated pointer of NDEF records in array of pointer
		delete[] rcds;	//free array of pointer
	}
}

uint32_t NdefMessage::write(uint8_t* out) {
	uint8_t* bhead = out;
	for (int i = 0; i < size; i++) {
		out += (*rcds[i]).writeRecord(out);
	}
	return (uint32_t) (out - bhead);
}



const NdefRecord* NdefMessage::getRecord(uint8_t idx) {
	return (const NdefRecord*) rcds[idx];
}


/*
 * @return sum of each length of record
 */
uint32_t NdefMessage::getSizeInByte() {
	uint32_t sz = 0;
	for (int i = 0; i < this->size; i++) {
		sz += (*rcds[i]).getRecordSize();
	}
	return sz;
}

}
